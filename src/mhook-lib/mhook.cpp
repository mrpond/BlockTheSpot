//Copyright (c) 2007-2008, Marton Anka
//
//Permission is hereby granted, free of charge, to any person obtaining a 
//copy of this software and associated documentation files (the "Software"), 
//to deal in the Software without restriction, including without limitation 
//the rights to use, copy, modify, merge, publish, distribute, sublicense, 
//and/or sell copies of the Software, and to permit persons to whom the 
//Software is furnished to do so, subject to the following conditions:
//
//The above copyright notice and this permission notice shall be included 
//in all copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
//OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
//THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
//FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
//IN THE SOFTWARE.

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "mhook.h"
#include "../disasm-lib/disasm.h"

//=========================================================================
#ifndef GOOD_HANDLE
#define GOOD_HANDLE(a) ((a!=INVALID_HANDLE_VALUE)&&(a!=NULL))
#endif

//=========================================================================
#ifndef gle
#define gle GetLastError
#endif

//=========================================================================
#ifndef ODPRINTF

#ifdef _DEBUG
#define ODPRINTF(a) odprintf a
#else
#define ODPRINTF(a)
#endif

inline void __cdecl odprintf(PCWSTR format, ...) 
{
    va_list args;
    va_start(args, format);
    int len = _vscwprintf(format, args);
    if (len > 0) 
    {
        len += (1 + 2);
        PWSTR buf = (PWSTR) malloc(sizeof(WCHAR)*len);
        if (buf) 
        {
            len = vswprintf_s(buf, len, format, args);
            if (len > 0) 
            {
                while (len && iswspace(buf[len-1])) len--;
                buf[len++] = L'\r';
                buf[len++] = L'\n';
                buf[len] = 0;
                OutputDebugStringW(buf);
            }
            free(buf);
        }
        va_end(args);
    }
}

#endif //#ifndef ODPRINTF

//=========================================================================
#define MHOOKS_MAX_CODE_BYTES   32
#define MHOOKS_MAX_RIPS          4

//=========================================================================
// The trampoline structure - stores every bit of info about a hook
struct MHOOKS_TRAMPOLINE 
{
    PBYTE   pSystemFunction;                                // the original system function
    DWORD   cbOverwrittenCode;                              // number of bytes overwritten by the jump
    PBYTE   pHookFunction;                                  // the hook function that we provide
    BYTE    codeJumpToHookFunction[MHOOKS_MAX_CODE_BYTES];  // placeholder for code that jumps to the hook function
    BYTE    codeTrampoline[MHOOKS_MAX_CODE_BYTES];          // placeholder for code that holds the first few
                                                            //   bytes from the system function and a jump to the remainder
                                                            //   in the original location
    BYTE    codeUntouched[MHOOKS_MAX_CODE_BYTES];           // placeholder for unmodified original code
                                                            //   (we patch IP-relative addressing)
    MHOOKS_TRAMPOLINE* pPrevTrampoline;                     // When in the free list, thess are pointers to the prev and next entry.
    MHOOKS_TRAMPOLINE* pNextTrampoline;                     // When not in the free list, this is a pointer to the prev and next trampoline in use.
};

//=========================================================================
// The patch data structures - store info about rip-relative instructions
// during hook placement
struct MHOOKS_RIPINFO
{
    DWORD   dwOffset;
    S64     nDisplacement;
};

struct MHOOKS_PATCHDATA
{
    S64             nLimitUp;
    S64             nLimitDown;
    DWORD           nRipCnt;
    MHOOKS_RIPINFO  rips[MHOOKS_MAX_RIPS];
};

//=========================================================================
// Hook context contains info about one hook
struct HOOK_CONTEXT
{
    PVOID pSystemFunction;
    PVOID pHookFunction;
    DWORD dwInstructionLength;
    MHOOKS_TRAMPOLINE* pTrampoline;

    MHOOKS_PATCHDATA patchdata;

    bool needPatchJump;
    bool needPatchCall;
};

//=========================================================================
// Global vars
static bool g_bVarsInitialized = false;
static CRITICAL_SECTION g_cs;
static MHOOKS_TRAMPOLINE* g_pHooks = NULL;
static MHOOKS_TRAMPOLINE* g_pFreeList = NULL;
static HANDLE* g_hThreadHandles = NULL;
static DWORD g_nThreadHandles = 0;
#define MHOOK_JMPSIZE 5
#define MHOOK_MINALLOCSIZE 4096

//=========================================================================
// ntdll definitions

typedef LONG NTSTATUS;

typedef LONG KPRIORITY;

typedef enum _SYSTEM_INFORMATION_CLASS 
{
    SystemBasicInformation = 0,
    SystemPerformanceInformation = 2,
    SystemTimeOfDayInformation = 3,
    SystemProcessInformation = 5,
    SystemProcessorPerformanceInformation = 8,
    SystemHandleInformation = 16,
    SystemInterruptInformation = 23,
    SystemExceptionInformation = 33,
    SystemRegistryQuotaInformation = 37,
    SystemLookasideInformation = 45,
    SystemProcessIdInformation = 0x58
} SYSTEM_INFORMATION_CLASS;

typedef enum _KWAIT_REASON
{
    Executive,
    FreePage,
    PageIn,
    PoolAllocation,
    DelayExecution,
    Suspended,
    UserRequest,
    WrExecutive,
    WrFreePage,
    WrPageIn,
    WrPoolAllocation,
    WrDelayExecution,
    WrSuspended,
    WrUserRequest,
    WrEventPair,
    WrQueue,
    WrLpcReceive,
    WrLpcReply,
    WrVirtualMemory,
    WrPageOut,
    WrRendezvous,
    Spare2,
    Spare3,
    Spare4,
    Spare5,
    Spare6,
    WrKernel,
    MaximumWaitReason
} KWAIT_REASON, *PKWAIT_REASON;

typedef struct _CLIENT_ID
{
    HANDLE  UniqueProcess;
    HANDLE  UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

typedef struct _SYSTEM_THREAD_INFORMATION
{
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER CreateTime;
    ULONG WaitTime;
    PVOID StartAddress;
    CLIENT_ID ClientId;
    KPRIORITY Priority;
    LONG BasePriority;
    ULONG ContextSwitches;
    ULONG ThreadState;
    KWAIT_REASON WaitReason;
} SYSTEM_THREAD_INFORMATION, *PSYSTEM_THREAD_INFORMATION;

typedef struct _UNICODE_STRING 
{
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING;

typedef struct _SYSTEM_PROCESS_INFORMATION
{
    ULONG uNext;
    ULONG uThreadCount;
    LARGE_INTEGER WorkingSetPrivateSize; // since VISTA
    ULONG HardFaultCount; // since WIN7
    ULONG NumberOfThreadsHighWatermark; // since WIN7
    ULONGLONG CycleTime; // since WIN7
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER KernelTime;
    UNICODE_STRING ImageName;
    KPRIORITY BasePriority;
    HANDLE uUniqueProcessId;
    HANDLE InheritedFromUniqueProcessId;
    ULONG HandleCount;
    ULONG SessionId;
    ULONG_PTR UniqueProcessKey; // since VISTA (requires SystemExtendedProcessInformation)
    SIZE_T PeakVirtualSize;
    SIZE_T VirtualSize;
    ULONG PageFaultCount;
    SIZE_T PeakWorkingSetSize;
    SIZE_T WorkingSetSize;
    SIZE_T QuotaPeakPagedPoolUsage;
    SIZE_T QuotaPagedPoolUsage;
    SIZE_T QuotaPeakNonPagedPoolUsage;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
    SIZE_T PrivatePageCount;
    LARGE_INTEGER ReadOperationCount;
    LARGE_INTEGER WriteOperationCount;
    LARGE_INTEGER OtherOperationCount;
    LARGE_INTEGER ReadTransferCount;
    LARGE_INTEGER WriteTransferCount;
    LARGE_INTEGER OtherTransferCount;
    SYSTEM_THREAD_INFORMATION Threads[1];
} SYSTEM_PROCESS_INFORMATION, *PSYSTEM_PROCESS_INFORMATION;

//=========================================================================
// ZwQuerySystemInformation definitions
typedef NTSTATUS(NTAPI* PZwQuerySystemInformation)(
    __in       SYSTEM_INFORMATION_CLASS SystemInformationClass,
    __inout    PVOID SystemInformation,
    __in       ULONG SystemInformationLength,
    __out_opt  PULONG ReturnLength
    );

#define STATUS_INFO_LENGTH_MISMATCH      ((NTSTATUS)0xC0000004L)
static PZwQuerySystemInformation fnZwQuerySystemInformation = reinterpret_cast<PZwQuerySystemInformation>(GetProcAddress(GetModuleHandle("ntdll.dll"), "ZwQuerySystemInformation"));

//=========================================================================
// Internal function:
//
// Remove the trampoline from the specified list, updating the head pointer
// if necessary.
//=========================================================================
static VOID ListRemove(MHOOKS_TRAMPOLINE** pListHead, MHOOKS_TRAMPOLINE* pNode) 
{
    if (pNode->pPrevTrampoline) 
    {
        pNode->pPrevTrampoline->pNextTrampoline = pNode->pNextTrampoline;
    }

    if (pNode->pNextTrampoline) 
    {
        pNode->pNextTrampoline->pPrevTrampoline = pNode->pPrevTrampoline;
    }

    if ((*pListHead) == pNode) 
    {
        (*pListHead) = pNode->pNextTrampoline;
        if (*pListHead != NULL) 
        {
            assert((*pListHead)->pPrevTrampoline == NULL);
        }
    }

    pNode->pPrevTrampoline = NULL;
    pNode->pNextTrampoline = NULL;
}

//=========================================================================
// Internal function:
//
// Prepend the trampoline from the specified list and update the head pointer.
//=========================================================================
static VOID ListPrepend(MHOOKS_TRAMPOLINE** pListHead, MHOOKS_TRAMPOLINE* pNode) 
{
    pNode->pPrevTrampoline = NULL;
    pNode->pNextTrampoline = (*pListHead);
    if ((*pListHead)) 
    {
        (*pListHead)->pPrevTrampoline = pNode;
    }
    (*pListHead) = pNode;
}

//=========================================================================
// Internal function:
//
// For iteration over the list
//=========================================================================
static MHOOKS_TRAMPOLINE* ListNext(MHOOKS_TRAMPOLINE* pNode)
{
    return pNode && pNode->pNextTrampoline ? pNode->pNextTrampoline : NULL;
}

//=========================================================================
static VOID EnterCritSec() 
{
    if (!g_bVarsInitialized) 
    {
        InitializeCriticalSection(&g_cs);
        g_bVarsInitialized = true;
    }
    EnterCriticalSection(&g_cs);
}

//=========================================================================
static VOID LeaveCritSec() 
{
    LeaveCriticalSection(&g_cs);
}

//=========================================================================
// Internal function:
// 
// Skip over jumps that lead to the real function. Gets around import
// jump tables, etc.
//=========================================================================
static PBYTE SkipJumps(PBYTE pbCode) 
{
    PBYTE pbOrgCode = pbCode;
#ifdef _M_IX86_X64
#ifdef _M_IX86
    //mov edi,edi: hot patch point
    if (pbCode[0] == 0x8b && pbCode[1] == 0xff)
        pbCode += 2;
    // push ebp; mov ebp, esp; pop ebp;
    // "collapsed" stackframe generated by MSVC
    if (pbCode[0] == 0x55 && pbCode[1] == 0x8b && pbCode[2] == 0xec && pbCode[3] == 0x5d)
        pbCode += 4;
#endif  
    if (pbCode[0] == 0xff && pbCode[1] == 0x25) 
    {
#ifdef _M_IX86
        // on x86 we have an absolute pointer...
        PBYTE pbTarget = *(PBYTE *)&pbCode[2];
        // ... that shows us an absolute pointer.
        return SkipJumps(*(PBYTE *)pbTarget);
#elif defined _M_X64
        // on x64 we have a 32-bit offset...
        INT32 lOffset = *(INT32 *)&pbCode[2];
        // ... that shows us an absolute pointer
        return SkipJumps(*(PBYTE*)(pbCode + 6 + lOffset));
    } 
    else if (pbCode[0] == 0x48 && pbCode[1] == 0xff && pbCode[2] == 0x25) 
    {
        // or we can have the same with a REX prefix
        INT32 lOffset = *(INT32 *)&pbCode[3];
        // ... that shows us an absolute pointer
        return SkipJumps(*(PBYTE*)(pbCode + 7 + lOffset));
#endif
    } 
    else if (pbCode[0] == 0xe9) 
    {
        // here the behavior is identical, we have...
        // ...a 32-bit offset to the destination.
        return SkipJumps(pbCode + 5 + *(INT32 *)&pbCode[1]);
    } 
    else if (pbCode[0] == 0xeb) 
    {
        // and finally an 8-bit offset to the destination
        return SkipJumps(pbCode + 2 + *(CHAR *)&pbCode[1]);
    }
#else
#error unsupported platform
#endif
    return pbOrgCode;
}

//=========================================================================
// Internal function:
//
// Writes code at pbCode that jumps to pbJumpTo. Will attempt to do this
// in as few bytes as possible. Important on x64 where the long jump
// (0xff 0x25 ....) can take up 14 bytes.
//=========================================================================
static PBYTE EmitJump(PBYTE pbCode, PBYTE pbJumpTo) 
{
#ifdef _M_IX86_X64
    PBYTE pbJumpFrom = pbCode + 5;
    SIZE_T cbDiff = pbJumpFrom > pbJumpTo ? pbJumpFrom - pbJumpTo : pbJumpTo - pbJumpFrom;
    ODPRINTF((L"mhooks: EmitJump: Jumping from %p to %p, diff is %p", pbJumpFrom, pbJumpTo, cbDiff));
    if (cbDiff <= 0x7fff0000) 
    {
        pbCode[0] = 0xe9;
        pbCode += 1;
        *((PDWORD)pbCode) = (DWORD)(DWORD_PTR)(pbJumpTo - pbJumpFrom);
        pbCode += sizeof(DWORD);
    } 
    else 
    {
        pbCode[0] = 0xff;
        pbCode[1] = 0x25;
        pbCode += 2;
#ifdef _M_IX86
        // on x86 we write an absolute address (just behind the instruction)
        *((PDWORD)pbCode) = (DWORD)(DWORD_PTR)(pbCode + sizeof(DWORD));
#elif defined _M_X64
        // on x64 we write the relative address of the same location
        *((PDWORD)pbCode) = (DWORD)0;
#endif
        pbCode += sizeof(DWORD);
        *((PDWORD_PTR)pbCode) = (DWORD_PTR)(pbJumpTo);
        pbCode += sizeof(DWORD_PTR);
    }
#else 
#error unsupported platform
#endif
    return pbCode;
}

//=========================================================================
// Internal function:
//
// Round down to the next multiple of rndDown
//=========================================================================
static size_t RoundDown(size_t addr, size_t rndDown)
{
    return (addr / rndDown) * rndDown;
}

//=========================================================================
// Internal function:
//
// Will attempt allocate a block of memory within the specified range, as 
// near as possible to the specified function.
//=========================================================================
static MHOOKS_TRAMPOLINE* BlockAlloc(PBYTE pSystemFunction, PBYTE pbLower, PBYTE pbUpper) 
{
    SYSTEM_INFO sSysInfo =  {0};
    ::GetSystemInfo(&sSysInfo);

    // Always allocate in bulk, in case the system actually has a smaller allocation granularity than MINALLOCSIZE.
    const ptrdiff_t cAllocSize = MAX(sSysInfo.dwAllocationGranularity, MHOOK_MINALLOCSIZE);

    MHOOKS_TRAMPOLINE* pRetVal = NULL;
    PBYTE pModuleGuess = (PBYTE) RoundDown((size_t)pSystemFunction, cAllocSize);
    int loopCount = 0;
    for (PBYTE pbAlloc = pModuleGuess; pbLower < pbAlloc && pbAlloc < pbUpper; ++loopCount) 
    {
        // determine current state
        MEMORY_BASIC_INFORMATION mbi;
        ODPRINTF((L"mhooks: BlockAlloc: Looking at address %p", pbAlloc));
        if (!VirtualQuery(pbAlloc, &mbi, sizeof(mbi)))
            break;
        // free & large enough?
        if (mbi.State == MEM_FREE && mbi.RegionSize >= (unsigned)cAllocSize) 
        {
            // and then try to allocate it
            pRetVal = (MHOOKS_TRAMPOLINE*)VirtualAlloc(pbAlloc, cAllocSize, MEM_COMMIT|MEM_RESERVE, PAGE_EXECUTE_READWRITE);
            if (pRetVal) 
            {
                size_t trampolineCount = cAllocSize / sizeof(MHOOKS_TRAMPOLINE);
                ODPRINTF((L"mhooks: BlockAlloc: Allocated block at %p as %d trampolines", pRetVal, trampolineCount));

                pRetVal[0].pPrevTrampoline = NULL;
                pRetVal[0].pNextTrampoline = &pRetVal[1];

                // prepare them by having them point down the line at the next entry.
                for (size_t s = 1; s < trampolineCount; ++s) 
                {
                    pRetVal[s].pPrevTrampoline = &pRetVal[s - 1];
                    pRetVal[s].pNextTrampoline = &pRetVal[s + 1];
                }

                // last entry points to the current head of the free list
                pRetVal[trampolineCount - 1].pNextTrampoline = g_pFreeList;
                if (g_pFreeList) 
                {
                    g_pFreeList->pPrevTrampoline = &pRetVal[trampolineCount - 1];
                }
                break;
            }
        }
                
        // This is a spiral, should be -1, 1, -2, 2, -3, 3, etc. (* cAllocSize)
        ptrdiff_t bytesToOffset = (cAllocSize * (loopCount + 1) * ((loopCount % 2 == 0) ? -1 : 1));
        pbAlloc = pbAlloc + bytesToOffset;
    }
    
    return pRetVal;
}

//=========================================================================
// Internal function:
//
// Will try to allocate a big block of memory inside the required range. 
//=========================================================================
static MHOOKS_TRAMPOLINE* FindTrampolineInRange(PBYTE pLower, PBYTE pUpper) 
{
    if (!g_pFreeList) 
    {
        return NULL;
    }

    // This is a standard free list, except we're doubly linked to deal with soem return shenanigans.
    MHOOKS_TRAMPOLINE* curEntry = g_pFreeList;
    while (curEntry) 
    {
        if ((MHOOKS_TRAMPOLINE*) pLower < curEntry && curEntry < (MHOOKS_TRAMPOLINE*) pUpper) 
        {
            ListRemove(&g_pFreeList, curEntry);

            return curEntry;
        }

        curEntry = curEntry->pNextTrampoline;
    }

    return NULL;
}

//=========================================================================
// Internal function:
//
// Will try to allocate the trampoline structure within 2 gigabytes of
// the target function. 
//=========================================================================
static MHOOKS_TRAMPOLINE* TrampolineAlloc(PBYTE pSystemFunction, S64 nLimitUp, S64 nLimitDown) 
{

    MHOOKS_TRAMPOLINE* pTrampoline = NULL;

    // determine lower and upper bounds for the allocation locations.
    // in the basic scenario this is +/- 2GB but IP-relative instructions
    // found in the original code may require a smaller window.
    PBYTE pLower = pSystemFunction + nLimitUp;
    pLower = pLower < (PBYTE)(DWORD_PTR)0x0000000080000000 ? 
                        (PBYTE)(0x1) : (PBYTE)(pLower - (PBYTE)0x7fff0000);
    PBYTE pUpper = pSystemFunction + nLimitDown;
    pUpper = pUpper < (PBYTE)(DWORD_PTR)0xffffffff80000000 ? 
        (PBYTE)(pUpper + (DWORD_PTR)0x7ff80000) : (PBYTE)(DWORD_PTR)0xfffffffffff80000;
    ODPRINTF((L"mhooks: TrampolineAlloc: Allocating for %p between %p and %p", pSystemFunction, pLower, pUpper));

    // try to find a trampoline in the specified range
    pTrampoline = FindTrampolineInRange(pLower, pUpper);
    if (!pTrampoline) 
    {
        // if it we can't find it, then we need to allocate a new block and 
        // try again. Just fail if that doesn't work 
        g_pFreeList = BlockAlloc(pSystemFunction, pLower, pUpper);
        pTrampoline = FindTrampolineInRange(pLower, pUpper);
    }

    // found and allocated a trampoline?
    if (pTrampoline) 
    {
        ListPrepend(&g_pHooks, pTrampoline);
    }

    return pTrampoline;
}

//=========================================================================
// Internal function:
//
// Return the internal trampoline structure that belongs to a hooked function.
//=========================================================================
static MHOOKS_TRAMPOLINE* TrampolineGet(PBYTE pHookedFunction) 
{
    MHOOKS_TRAMPOLINE* pCurrent = g_pHooks;

    while (pCurrent) 
    {
        if ((PBYTE)&(pCurrent->codeTrampoline) == pHookedFunction) 
        {
            return pCurrent;
        }

        pCurrent = pCurrent->pNextTrampoline;
    }

    return NULL;
}

//=========================================================================
// Internal function:
//
// Free a trampoline structure.
//=========================================================================
static VOID TrampolineFree(MHOOKS_TRAMPOLINE* pTrampoline, bool bNeverUsed) 
{
    ListRemove(&g_pHooks, pTrampoline);

    // If a thread could feasinbly have some of our trampoline code 
    // on its stack and we yank the region from underneath it then it will
    // surely crash upon returning. So instead of freeing the 
    // memory we just let it leak. Ugly, but safe.
    if (bNeverUsed) 
    {
        ListPrepend(&g_pFreeList, pTrampoline);
    }
}

static bool VerifyThreadContext(PBYTE pIp, HOOK_CONTEXT* hookCtx, int hookCount)
{
    for (int i = 0; i < hookCount; i++)
    {
        if (pIp >= (PBYTE)hookCtx[i].pSystemFunction && pIp < ((PBYTE)hookCtx[i].pSystemFunction + hookCtx[i].dwInstructionLength))
        {
            return false;
        }
    }

    return true;
}

//=========================================================================
// Internal function:
//
// Suspend a given thread and try to make sure that its instruction
// pointer is not in the given range.
//=========================================================================
//=========================================================================
static HANDLE SuspendOneThread(DWORD dwThreadId, HOOK_CONTEXT* hookCtx, int hookCount)
{
    // open the thread
    HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, false, dwThreadId);

    if (GOOD_HANDLE(hThread))
    {
        // attempt suspension
        DWORD dwSuspendCount = SuspendThread(hThread);
        if (dwSuspendCount != -1)
        {
            // see where the IP is
            CONTEXT ctx;
            ctx.ContextFlags = CONTEXT_CONTROL;
            int nTries = 0;
            while (GetThreadContext(hThread, &ctx))
            {
#ifdef _M_IX86
                PBYTE pIp = (PBYTE)(DWORD_PTR)ctx.Eip;
#elif defined _M_X64
                PBYTE pIp = (PBYTE)(DWORD_PTR)ctx.Rip;
#endif
                if (!VerifyThreadContext(pIp, hookCtx, hookCount))
                {
                    if (nTries < 3)
                    {
                        // oops - we should try to get the instruction pointer out of here. 
                        ODPRINTF((L"mhooks: SuspendOneThread: suspended thread %d - IP is at %p - IS COLLIDING WITH CODE", dwThreadId, pIp));
                        ResumeThread(hThread);
                        Sleep(100);
                        SuspendThread(hThread);
                        nTries++;
                    }
                    else
                    {
                        // we gave it all we could. (this will probably never 
                        // happen - unless the thread has already been suspended 
                        // to begin with)
                        ODPRINTF((L"mhooks: SuspendOneThread: suspended thread %d - IP is at %p - IS COLLIDING WITH CODE - CAN'T FIX", dwThreadId, pIp));
                        ResumeThread(hThread);
                        CloseHandle(hThread);
                        hThread = NULL;
                        break;
                    }
                }
                else
                {
                    // success, the IP is not conflicting
                    ODPRINTF((L"mhooks: SuspendOneThread: Successfully suspended thread %d - IP is at %p", dwThreadId, pIp));
                    break;
                }
            }
        }
        else
        {
            // couldn't suspend
            CloseHandle(hThread);
            hThread = NULL;
        }
    }

    return hThread;
}

//=========================================================================
// Internal function:
//
// Free memory allocated for processes snapshot
//=========================================================================
static VOID CloseProcessSnapshot(VOID* snapshotContext)
{
    free(snapshotContext);
}

//=========================================================================
// Internal function:
//
// Resumes all previously suspended threads in the current process.
//=========================================================================
static VOID ResumeOtherThreads() 
{
    // make sure things go as fast as possible
    INT nOriginalPriority = GetThreadPriority(GetCurrentThread());
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
    // go through our list
    for (DWORD i=0; i<g_nThreadHandles; i++) 
    {
        // and resume & close thread handles
        ResumeThread(g_hThreadHandles[i]);
        CloseHandle(g_hThreadHandles[i]);
    }
    // clean up
    free(g_hThreadHandles);
    g_hThreadHandles = NULL;
    g_nThreadHandles = 0;
    SetThreadPriority(GetCurrentThread(), nOriginalPriority);
}

//=========================================================================
// Internal function:
//
// Get snapshot of the processes started in the system
//=========================================================================
static bool CreateProcessSnapshot(VOID** snapshotContext)
{
    ULONG   cbBuffer = 1024 * 1024;  // 1Mb - default process information buffer size (that's enough in most cases for high-loaded systems)
    LPVOID  pBuffer = NULL;
    NTSTATUS status = 0;

    do
    {
        pBuffer = malloc(cbBuffer);
        if (pBuffer == NULL)
        {
            return false;
        }

        status = fnZwQuerySystemInformation(SystemProcessInformation, pBuffer, cbBuffer, NULL);

        if (status == STATUS_INFO_LENGTH_MISMATCH)
        {
            free(pBuffer);
            cbBuffer *= 2;
        }
        else if (status < 0)
        {
            free(pBuffer);
            return false;
        }
    } while (status == STATUS_INFO_LENGTH_MISMATCH);

    *snapshotContext = pBuffer;

    return true;
}

//=========================================================================
// Internal function:
//
// Find and return process information from snapshot
//=========================================================================
static PSYSTEM_PROCESS_INFORMATION FindProcess(VOID* snapshotContext, SIZE_T processId)
{
    PSYSTEM_PROCESS_INFORMATION currentProcess = (PSYSTEM_PROCESS_INFORMATION)snapshotContext;

    while (currentProcess != NULL)
    {
        if (currentProcess->uUniqueProcessId == (HANDLE)processId)
        {
            break;
        }

        if (currentProcess->uNext == 0)
        {
            currentProcess = NULL;
        }
        else
        {
            currentProcess = (PSYSTEM_PROCESS_INFORMATION)(((LPBYTE)currentProcess) + currentProcess->uNext);
        }
    }

    return currentProcess;
}

//=========================================================================
// Internal function:
//
// Get current process snapshot and process info
//
//=========================================================================
static bool GetCurrentProcessSnapshot(PVOID* snapshot, PSYSTEM_PROCESS_INFORMATION* procInfo)
{
    // get a view of the threads in the system

    if (!CreateProcessSnapshot(snapshot))
    {
        ODPRINTF((L"mhooks: can't get process snapshot!"));
        return false;
    }

    DWORD pid = GetCurrentProcessId();

    *procInfo = FindProcess(*snapshot, pid);
    return true;
}

//=========================================================================
// Internal function:
//
// Suspend all threads in this process while trying to make sure that their 
// instruction pointer is not in the given range.
//=========================================================================
static bool SuspendOtherThreads(HOOK_CONTEXT* hookCtx, int hookCount, PSYSTEM_PROCESS_INFORMATION procInfo) 
{
    bool bRet = false;
    // make sure we're the most important thread in the process
    INT nOriginalPriority = GetThreadPriority(GetCurrentThread());
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

    DWORD pid = GetCurrentProcessId();
    DWORD tid = GetCurrentThreadId();

    // count threads in this process (except for ourselves)
    DWORD nThreadsInProcess = 0;

    if (procInfo->uThreadCount != 0)
    {
        nThreadsInProcess = procInfo->uThreadCount - 1;
    }

    ODPRINTF((L"mhooks: [%d:%d] SuspendOtherThreads: counted %d other threads", pid, tid, nThreadsInProcess));

    if (nThreadsInProcess)
    {
        // alloc buffer for the handles we really suspended
        g_hThreadHandles = (HANDLE*)malloc(nThreadsInProcess * sizeof(HANDLE));

        if (g_hThreadHandles)
        {
            ZeroMemory(g_hThreadHandles, nThreadsInProcess * sizeof(HANDLE));
            DWORD nCurrentThread = 0;
            bool bFailed = false;

            // go through every thread
            for (ULONG threadIdx = 0; threadIdx < procInfo->uThreadCount; threadIdx++)
            {
                DWORD threadId = static_cast<DWORD>(reinterpret_cast<DWORD_PTR>(procInfo->Threads[threadIdx].ClientId.UniqueThread));

                if (threadId != tid)
                {
                    // attempt to suspend it
                    g_hThreadHandles[nCurrentThread] = SuspendOneThread(threadId, hookCtx, hookCount);

                    if (GOOD_HANDLE(g_hThreadHandles[nCurrentThread]))
                    {
                        ODPRINTF((L"mhooks: [%d:%d] SuspendOtherThreads: successfully suspended %d", pid, tid, threadId));
                        nCurrentThread++;
                    }
                    else
                    {
                        ODPRINTF((L"mhooks: [%d:%d] SuspendOtherThreads: error while suspending thread %d: %d", pid, tid, threadId, gle()));
                        // TODO: this might not be the wisest choice
                        // but we can choose to ignore failures on
                        // thread suspension. It's pretty unlikely that
                        // we'll fail - and even if we do, the chances
                        // of a thread's IP being in the wrong place
                        // is pretty small.
                        // bFailed = true;
                    }
                }
            }

            g_nThreadHandles = nCurrentThread;
            bRet = !bFailed;
        }
    }    

    //TODO: we might want to have another pass to make sure all threads
    // in the current process (including those that might have been
    // created since we took the original snapshot) have been 
    // suspended.

    if (!bRet && nThreadsInProcess != 0)
    {
        ODPRINTF((L"mhooks: [%d:%d] SuspendOtherThreads: Had a problem (or not running multithreaded), resuming all threads.", pid, tid));
        ResumeOtherThreads();
    }

    SetThreadPriority(GetCurrentThread(), nOriginalPriority);

    return bRet;
}

//=========================================================================
// if IP-relative addressing has been detected, fix up the code so the
// offset points to the original location
static void FixupIPRelativeAddressing(PBYTE pbNew, PBYTE pbOriginal, MHOOKS_PATCHDATA* pdata)
{
#if defined _M_X64
    S64 diff = pbNew - pbOriginal;
    for (DWORD i = 0; i < pdata->nRipCnt; i++) 
    {
        DWORD dwNewDisplacement = (DWORD)(pdata->rips[i].nDisplacement - diff);
        ODPRINTF((L"mhooks: fixing up RIP instruction operand for code at 0x%p: "
            L"old displacement: 0x%8.8x, new displacement: 0x%8.8x", 
            pbNew + pdata->rips[i].dwOffset, 
            (DWORD)pdata->rips[i].nDisplacement, 
            dwNewDisplacement));
        *(PDWORD)(pbNew + pdata->rips[i].dwOffset) = dwNewDisplacement;
    }
#endif
}

//=========================================================================
// Examine the machine code at the target function's entry point, and
// skip bytes in a way that we'll always end on an instruction boundary.
// We also detect branches and subroutine calls (as well as returns)
// at which point disassembly must stop.
// Finally, detect and collect information on IP-relative instructions
// that we can patch.
static DWORD DisassembleAndSkip(PVOID pFunction, DWORD dwMinLen, MHOOKS_PATCHDATA* pdata) 
{
    DWORD dwRet = 0;
    pdata->nLimitDown = 0;
    pdata->nLimitUp = 0;
    pdata->nRipCnt = 0;
#ifdef _M_IX86
    ARCHITECTURE_TYPE arch = ARCH_X86;
#elif defined _M_X64
    ARCHITECTURE_TYPE arch = ARCH_X64;
#else
    #error unsupported platform
#endif
    DISASSEMBLER dis;
    if (InitDisassembler(&dis, arch)) 
    {
        INSTRUCTION* pins = NULL;
        U8* pLoc = (U8*)pFunction;
        DWORD dwFlags = DISASM_DECODE | DISASM_DISASSEMBLE | DISASM_ALIGNOUTPUT;

        ODPRINTF((L"mhooks: DisassembleAndSkip: Disassembling %p", pLoc));
        while ( (dwRet < dwMinLen) && (pins = GetInstruction(&dis, (ULONG_PTR)pLoc, pLoc, dwFlags)) ) 
        {
            ODPRINTF((L"mhooks: DisassembleAndSkip: %p:(0x%2.2x) %s", pLoc, pins->Length, pins->String));
            if (pins->Type == ITYPE_RET     )   break;
            if (pins->Type == ITYPE_BRANCHCC)   break;
            if (pins->Type == ITYPE_CALLCC)     break;
            
            #if defined _M_X64
                bool bProcessRip = false;
                // jmp to rip+imm32
                if ((pins->Type == ITYPE_BRANCH) && (pins->OperandCount == 1) && (pins->X86.Relative) && (pins->X86.BaseRegister == AMD64_REG_RIP) && (pins->Operands[0].Flags & OP_IPREL))
                {
                    // rip-addressing "jmp [rip+imm32]"
                    ODPRINTF((L"mhooks: DisassembleAndSkip: found OP_IPREL on operand %d with displacement 0x%x (in memory: 0x%x)", 1, pins->X86.Displacement, *(PDWORD)(pLoc + 3)));
                    bProcessRip = true;
                }
            
                // mov or lea to register from rip+imm32
                else if ((pins->Type == ITYPE_MOV || pins->Type == ITYPE_LEA) && (pins->X86.Relative) && 
                    (pins->X86.OperandSize == 8) && (pins->OperandCount == 2) &&
                    (pins->Operands[1].Flags & OP_IPREL) && (pins->Operands[1].Register == AMD64_REG_RIP))
                {
                    // rip-addressing "mov reg, [rip+imm32]"
                    ODPRINTF((L"mhooks: DisassembleAndSkip: found OP_IPREL on operand %d with displacement 0x%x (in memory: 0x%x)", 1, pins->X86.Displacement, *(PDWORD)(pLoc+3)));
                    bProcessRip = true;
                }
                // mov or lea to rip+imm32 from register
                else if ((pins->Type == ITYPE_MOV || pins->Type == ITYPE_LEA) && (pins->X86.Relative) && 
                    (pins->X86.OperandSize == 8) && (pins->OperandCount == 2) &&
                    (pins->Operands[0].Flags & OP_IPREL) && (pins->Operands[0].Register == AMD64_REG_RIP))
                {
                    // rip-addressing "mov [rip+imm32], reg"
                    ODPRINTF((L"mhooks: DisassembleAndSkip: found OP_IPREL on operand %d with displacement 0x%x (in memory: 0x%x)", 0, pins->X86.Displacement, *(PDWORD)(pLoc+3)));
                    bProcessRip = true;
                }
                else if ( (pins->OperandCount >= 1) && (pins->Operands[0].Flags & OP_IPREL) )
                {
                    // unsupported rip-addressing
                    ODPRINTF((L"mhooks: DisassembleAndSkip: found unsupported OP_IPREL on operand %d", 0));
                    // dump instruction bytes to the debug output
                    for (DWORD i=0; i<pins->Length; i++) 
                    {
                        ODPRINTF((L"mhooks: DisassembleAndSkip: instr byte %2.2d: 0x%2.2x", i, pLoc[i]));
                    }
                    break;
                }
                else if ( (pins->OperandCount >= 2) && (pins->Operands[1].Flags & OP_IPREL) )
                {
                    // unsupported rip-addressing
                    ODPRINTF((L"mhooks: DisassembleAndSkip: found unsupported OP_IPREL on operand %d", 1));
                    // dump instruction bytes to the debug output
                    for (DWORD i=0; i<pins->Length; i++) 
                    {
                        ODPRINTF((L"mhooks: DisassembleAndSkip: instr byte %2.2d: 0x%2.2x", i, pLoc[i]));
                    }
                    break;
                }
                else if ( (pins->OperandCount >= 3) && (pins->Operands[2].Flags & OP_IPREL) )
                {
                    // unsupported rip-addressing
                    ODPRINTF((L"mhooks: DisassembleAndSkip: found unsupported OP_IPREL on operand %d", 2));
                    // dump instruction bytes to the debug output
                    for (DWORD i=0; i<pins->Length; i++) 
                    {
                        ODPRINTF((L"mhooks: DisassembleAndSkip: instr byte %2.2d: 0x%2.2x", i, pLoc[i]));
                    }
                    break;
                }
                // follow through with RIP-processing if needed
                if (bProcessRip) 
                {
                    // calculate displacement relative to function start
                    S64 nAdjustedDisplacement = pins->X86.Displacement + (pLoc - (U8*)pFunction);
                    // store displacement values furthest from zero (both positive and negative)
                    if (nAdjustedDisplacement < pdata->nLimitDown)
                        pdata->nLimitDown = nAdjustedDisplacement;
                    if (nAdjustedDisplacement > pdata->nLimitUp)
                        pdata->nLimitUp = nAdjustedDisplacement;
                    // store patch info
                    if (pdata->nRipCnt < MHOOKS_MAX_RIPS) 
                    {
                        pdata->rips[pdata->nRipCnt].dwOffset = dwRet + 3;
                        pdata->rips[pdata->nRipCnt].nDisplacement = pins->X86.Displacement;
                        pdata->nRipCnt++;
                    } 
                    else 
                    {
                        // no room for patch info, stop disassembly
                        break;
                    }
                }
            #endif

            dwRet += pins->Length;
            pLoc  += pins->Length;
        }

        CloseDisassembler(&dis);
    }

    return dwRet;
}

static bool IsInstructionPresentInFirstFiveByte(PVOID pFunction, INSTRUCTION_TYPE type)
{
    DWORD dwRet = 0;

#ifdef _M_IX86
    ARCHITECTURE_TYPE arch = ARCH_X86;
#elif defined _M_X64
    ARCHITECTURE_TYPE arch = ARCH_X64;
#else
#error unsupported platform
#endif
    DISASSEMBLER dis;
    if (InitDisassembler(&dis, arch))
    {
        INSTRUCTION* pins = NULL;
        U8* pLoc = (U8*)pFunction;
        DWORD dwFlags = DISASM_DECODE | DISASM_DISASSEMBLE | DISASM_ALIGNOUTPUT;

        while ((dwRet < MHOOK_JMPSIZE) && (pins = GetInstruction(&dis, (ULONG_PTR)pLoc, pLoc, dwFlags)))
        {
            if (pins->Type == type)
            {
                return true;
            }

            dwRet += pins->Length;
            pLoc += pins->Length;
        }

        CloseDisassembler(&dis);
    }

    return false;
}

static PBYTE PatchRelative(PBYTE pCodeTrampoline, PVOID pSystemFunction)
{
    DWORD dwRet = 0;

#ifdef _M_IX86
    ARCHITECTURE_TYPE arch = ARCH_X86;
#elif defined _M_X64
    ARCHITECTURE_TYPE arch = ARCH_X64;
#else
#error unsupported platform
#endif
    DISASSEMBLER dis;
    if (InitDisassembler(&dis, arch))
    {
        INSTRUCTION* pins = NULL;
        U8* pLoc = (U8*)pCodeTrampoline;
        DWORD dwFlags = DISASM_DECODE | DISASM_DISASSEMBLE | DISASM_ALIGNOUTPUT;

        while ((dwRet < MHOOK_JMPSIZE) && (pins = GetInstruction(&dis, (ULONG_PTR)pLoc, pLoc, dwFlags)))
        {
            if (pins->Type == ITYPE_BRANCHCC)
            {
                // we will patch only near jump je/jz for now
                if (pins->OpcodeLength == 1 && (pins->OpcodeBytes[0] == 0x74 || pins->OpcodeBytes[0] == 0x75))
                {
                    // save old offset from current position to jump destination
                    U8 oldOffset = *(pLoc + pins->OpcodeLength);
                    // write je opcode with rel32 address in codeTrampoline block
                    *pLoc = 0x0f;
                    *(pLoc + pins->OpcodeLength) = pins->OpcodeBytes[0] + 0x10; 
                    
                    // Calculating offset from codeTrampoline to jump label in original function

                    // get address of original jump destination
                    ULONG_PTR jumpDestinationAddress = reinterpret_cast<ULONG_PTR>(pSystemFunction);
                    // oldOffset is from the pLoc + pins->OpcodeLength address, so add it
                    jumpDestinationAddress += oldOffset + pins->OpcodeLength;

                    // current address is from the pLoc + 2 (je rel32 opcode is 2-bytes length), so add it
                    const DWORD kJERel32OpcodeLength = 2;
                    ULONG_PTR currentAddress = reinterpret_cast<ULONG_PTR>(pLoc + kJERel32OpcodeLength);

                    // take the offset that we should add to current address to reach original jump destination
                    LONG newOffset = static_cast<LONG>(jumpDestinationAddress - currentAddress);
                    assert(currentAddress + newOffset == jumpDestinationAddress);

                    memcpy(pLoc + kJERel32OpcodeLength, &newOffset, sizeof(newOffset));

                    return pLoc + kJERel32OpcodeLength + sizeof(newOffset);
                }
            }

            if (pins->Type == ITYPE_CALL)
            {
                // we will patch CALL relative32
                if (pins->OpcodeLength == 1 && pins->OpcodeBytes[0] == 0xE8)
                {
                    // call rel32 address is relative to the next instruction start address
                    // reinterpret_cast<ULONG_PTR>(pSystemFunction) is the original function address
                    // (pLoc - pCodeTrampoline) for current offset of call from start of the function,
                    // pins->Length - full legth of instruction and operand address
                    ULONG_PTR oldStartAddress = (pLoc - pCodeTrampoline) + reinterpret_cast<ULONG_PTR>(pSystemFunction)+pins->Length;
                    // offset from the next instruction address
                    LONG oldOffset = *(reinterpret_cast<LONG*>(pins->Operands[0].BCD));
                    // target function address
                    ULONG_PTR destination = oldStartAddress + oldOffset;

                    // now calculate new start address and new offset
                    ULONG_PTR newStartAddress = reinterpret_cast<ULONG_PTR>(pins->Address) + pins->Length;
                    LONG newOffset = static_cast<LONG>(destination - newStartAddress);

                    // save new offset to the trampoline code 
                    *reinterpret_cast<LONG*>(pLoc + pins->OpcodeLength) = newOffset;

                    return pLoc + pins->OpcodeLength + sizeof(newOffset);
                }
            }

            dwRet += pins->Length;
            pLoc += pins->Length;
        }

        CloseDisassembler(&dis);
    }

    return pCodeTrampoline;
}

static bool FindSystemFunction(HOOK_CONTEXT* hookCtx, int fromIdx, int toIdx, PVOID pSystemFunction)
{
    for (int idx = fromIdx; idx < toIdx; idx++)
    {
        if (hookCtx[idx].pSystemFunction == pSystemFunction)
        {
            return true;
        }
    }

    return false;
}

//=========================================================================
int Mhook_SetHookEx(HOOK_INFO* hooks, int hookCount)
{
    int hooksSet = 0;

    HOOK_CONTEXT* hookCtx = (HOOK_CONTEXT*)malloc(hookCount * sizeof(HOOK_CONTEXT));

    if (hookCtx == NULL)
    {
        // return error status
        ODPRINTF((L"mhooks: can't allocate buffer!"));

        return hooksSet;
    }

    EnterCritSec();

    for (int idx = 0; idx < hookCount; idx++)
    {
        hookCtx[idx].pSystemFunction = *hooks[idx].ppSystemFunction;
        hookCtx[idx].pHookFunction = hooks[idx].pHookFunction;
        hookCtx[idx].pTrampoline = NULL;
        hookCtx[idx].dwInstructionLength = 0;
        memset(&hookCtx[idx].patchdata, 0, sizeof(MHOOKS_PATCHDATA));
        hookCtx[idx].needPatchJump = false;
        hookCtx[idx].needPatchCall = false;

        ODPRINTF((L"mhooks: Mhook_SetHook: Started on the job: %p / %p", hookCtx[idx].pSystemFunction, hookCtx[idx].pHookFunction));

        // find the real functions (jump over jump tables, if any)
        hookCtx[idx].pSystemFunction = SkipJumps((PBYTE)hookCtx[idx].pSystemFunction);
        hookCtx[idx].pHookFunction = SkipJumps((PBYTE)hookCtx[idx].pHookFunction);

        if (FindSystemFunction(hookCtx, 0, idx, hookCtx[idx].pSystemFunction))
        {
            // Same system function found. Skip it.

            // It is not an error. 
            // This case is possible when two system functions from different DLLs are stubs and they are redirected to the one internal implementation.
            // We are going to hook first instance and skip other.

            ODPRINTF((L"mhooks: Mhook_SetHook: already hooked: %p", hookCtx[idx].pSystemFunction));

            hookCtx[idx].pTrampoline = NULL;
        }
        else
        {
            ODPRINTF((L"mhooks: Mhook_SetHook: Started on the job: %p / %p", hookCtx[idx].pSystemFunction, hookCtx[idx].pHookFunction));

            // figure out the length of the overwrite zone
            hookCtx[idx].dwInstructionLength = DisassembleAndSkip(hookCtx[idx].pSystemFunction, MHOOK_JMPSIZE, &hookCtx[idx].patchdata);

            hookCtx[idx].needPatchJump = IsInstructionPresentInFirstFiveByte(hookCtx[idx].pSystemFunction, ITYPE_BRANCHCC);
            hookCtx[idx].needPatchCall = IsInstructionPresentInFirstFiveByte(hookCtx[idx].pSystemFunction, ITYPE_CALL);
            
            if (hookCtx[idx].dwInstructionLength >= MHOOK_JMPSIZE && !(hookCtx[idx].needPatchJump && hookCtx[idx].needPatchCall))
            {
                ODPRINTF((L"mhooks: Mhook_SetHook: disassembly signals %d bytes", hookCtx[idx].dwInstructionLength));

                // allocate a trampoline structure (TODO: it is pretty wasteful to get
                // VirtualAlloc to grab chunks of memory smaller than 100 bytes)
                hookCtx[idx].pTrampoline = TrampolineAlloc((PBYTE)hookCtx[idx].pSystemFunction, hookCtx[idx].patchdata.nLimitUp, hookCtx[idx].patchdata.nLimitDown);
            }
            else
            {
                // error - skip hook
                ODPRINTF((L"mhooks: error! disassembly signals %d bytes (unacceptable)", hookCtx[idx].dwInstructionLength));
            }
        }
    }

    VOID* procEnumerationCtx = NULL;
    PSYSTEM_PROCESS_INFORMATION procInfo = NULL;

    if (GetCurrentProcessSnapshot(&procEnumerationCtx, &procInfo))
    {
        // suspend threads
        SuspendOtherThreads(hookCtx, hookCount, procInfo);

        // returns pseudo-handle, no need to CloseHandle() for it
        HANDLE currentProcessHandle = GetCurrentProcess();

        // the next code is same to the Mhook_SetHook.  Differences are only in using hookCtx[i]
        for (int i = 0; i < hookCount; i++)
        {
            if (hookCtx[i].pTrampoline)
            {
                ODPRINTF((L"mhooks: Mhook_SetHook: allocated structure at %p", hookCtx[i].pTrampoline));
                DWORD dwOldProtectSystemFunction = 0;
                DWORD dwOldProtectTrampolineFunction = 0;

                // set the system function to PAGE_EXECUTE_READWRITE
                if (VirtualProtect(hookCtx[i].pSystemFunction, hookCtx[i].dwInstructionLength, PAGE_EXECUTE_READWRITE, &dwOldProtectSystemFunction))
                {
                    ODPRINTF((L"mhooks: Mhook_SetHook: readwrite set on system function"));

                    // mark our trampoline buffer to PAGE_EXECUTE_READWRITE
                    if (VirtualProtect(hookCtx[i].pTrampoline, sizeof(MHOOKS_TRAMPOLINE), PAGE_EXECUTE_READWRITE, &dwOldProtectTrampolineFunction))
                    {
                        ODPRINTF((L"mhooks: Mhook_SetHook: readwrite set on trampoline structure"));

                        // create our trampoline function
                        PBYTE pbCode = hookCtx[i].pTrampoline->codeTrampoline;

                        // save original code..
                        for (DWORD k = 0; k < hookCtx[i].dwInstructionLength; k++) 
                        {
                            hookCtx[i].pTrampoline->codeUntouched[k] = pbCode[k] = ((PBYTE)hookCtx[i].pSystemFunction)[k];
                        }

                        if (hookCtx[i].needPatchJump || hookCtx[i].needPatchCall)
                        {
                            pbCode = PatchRelative(pbCode, hookCtx[i].pSystemFunction);
                        } 
                        else
                        {
                            pbCode += hookCtx[i].dwInstructionLength;
                        }

                        // plus a jump to the continuation in the original location
                        pbCode = EmitJump(pbCode, ((PBYTE)hookCtx[i].pSystemFunction) + hookCtx[i].dwInstructionLength);
                        ODPRINTF((L"mhooks: Mhook_SetHook: updated the trampoline"));

                        // fix up any IP-relative addressing in the code
                        FixupIPRelativeAddressing(hookCtx[i].pTrampoline->codeTrampoline, (PBYTE)hookCtx[i].pSystemFunction, &hookCtx[i].patchdata);

                        DWORD_PTR dwDistance = (PBYTE)hookCtx[i].pHookFunction < (PBYTE)hookCtx[i].pSystemFunction ?
                            (PBYTE)hookCtx[i].pSystemFunction - (PBYTE)hookCtx[i].pHookFunction : (PBYTE)hookCtx[i].pHookFunction - (PBYTE)hookCtx[i].pSystemFunction;

                        if (dwDistance > 0x7fff0000)
                        {
                            // create a stub that jumps to the replacement function.
                            // we need this because jumping from the API to the hook directly 
                            // will be a long jump, which is 14 bytes on x64, and we want to 
                            // avoid that - the API may or may not have room for such stuff. 
                            // (remember, we only have 5 bytes guaranteed in the API.)
                            // on the other hand we do have room, and the trampoline will always be
                            // within +/- 2GB of the API, so we do the long jump in there. 
                            // the API will jump to the "reverse trampoline" which
                            // will jump to the user's hook code.
                            pbCode = hookCtx[i].pTrampoline->codeJumpToHookFunction;
                            pbCode = EmitJump(pbCode, (PBYTE)hookCtx[i].pHookFunction);
                            ODPRINTF((L"mhooks: Mhook_SetHook: created reverse trampoline"));
                            FlushInstructionCache(GetCurrentProcess(), hookCtx[i].pTrampoline->codeJumpToHookFunction,
                                pbCode - hookCtx[i].pTrampoline->codeJumpToHookFunction);

                            // update the API itself
                            pbCode = (PBYTE)hookCtx[i].pSystemFunction;
                            pbCode = EmitJump(pbCode, hookCtx[i].pTrampoline->codeJumpToHookFunction);
                        }
                        else
                        {
                            // the jump will be at most 5 bytes so we can do it directly
                            // update the API itself
                            pbCode = (PBYTE)hookCtx[i].pSystemFunction;
                            pbCode = EmitJump(pbCode, (PBYTE)hookCtx[i].pHookFunction);
                        }

                        // update data members
                        hookCtx[i].pTrampoline->cbOverwrittenCode = hookCtx[i].dwInstructionLength;
                        hookCtx[i].pTrampoline->pSystemFunction = (PBYTE)hookCtx[i].pSystemFunction;
                        hookCtx[i].pTrampoline->pHookFunction = (PBYTE)hookCtx[i].pHookFunction;

                        // update pointer here for ability to hook system functions follows
                        if (hookCtx[i].pTrampoline->pSystemFunction)
                        {
                            // this is what the application will use as the entry point
                            // to the "original" unhooked function.
                            *hooks[i].ppSystemFunction = hookCtx[i].pTrampoline->codeTrampoline;
                        }

                        // flush instruction cache and restore original protection
                        FlushInstructionCache(currentProcessHandle, hookCtx[i].pTrampoline->codeTrampoline, hookCtx[i].dwInstructionLength);
                        VirtualProtect(hookCtx[i].pTrampoline, sizeof(MHOOKS_TRAMPOLINE), dwOldProtectTrampolineFunction, &dwOldProtectTrampolineFunction);
                    }
                    else
                    {
                        ODPRINTF((L"mhooks: Mhook_SetHook: failed VirtualProtect 2: %d", gle()));
                    }

                    // flush instruction cache and restore original protection
                    FlushInstructionCache(currentProcessHandle, hookCtx[i].pSystemFunction, hookCtx[i].dwInstructionLength);
                    VirtualProtect(hookCtx[i].pSystemFunction, hookCtx[i].dwInstructionLength, dwOldProtectSystemFunction, &dwOldProtectSystemFunction);
                }
                else
                {
                    ODPRINTF((L"mhooks: Mhook_SetHook: failed VirtualProtect 1: %d", gle()));
                }

                if (hookCtx[i].pTrampoline->pSystemFunction)
                {
                    hooksSet++;
                    // setting the entry point is moved upper for ability to hook some internal system functions
                    ODPRINTF((L"mhooks: Mhook_SetHook: Hooked the function!"));
                }
                else
                {
                    // if we failed discard the trampoline (forcing VirtualFree)
                    TrampolineFree(hookCtx[i].pTrampoline, true);
                    hookCtx[i].pTrampoline = NULL;
                }
            }
        }

        // resume threads
        ResumeOtherThreads();

        CloseProcessSnapshot(procEnumerationCtx);
    }

    free(hookCtx);

    LeaveCritSec();

    return hooksSet;
}

//=========================================================================
BOOL Mhook_SetHook(PVOID *ppSystemFunction, PVOID pHookFunction) 
{
    HOOK_INFO hook = { ppSystemFunction, pHookFunction };
    return Mhook_SetHookEx(&hook, 1) == 1;
}

//=========================================================================
int Mhook_UnhookEx(PVOID** hooks, int hookCount)
{
    ODPRINTF((L"mhooks: Mhook_UnhookEx: %d hooks to unhook", hookCount));
    int result = 0;

    HOOK_CONTEXT* hookCtx = (HOOK_CONTEXT*)malloc(hookCount * sizeof(HOOK_CONTEXT));
    if (hookCtx == NULL)
    {
        // return error status
        ODPRINTF((L"mhooks: Mhook_UnhookEx: can't allocate buffer!"));

        return result;
    }

    EnterCritSec();

    for (int idx = 0; idx < hookCount; idx++)
    {
        hookCtx[idx].pSystemFunction = *hooks[idx];
        // get the trampoline structure that corresponds to our function
        hookCtx[idx].pTrampoline = TrampolineGet((PBYTE)hookCtx[idx].pSystemFunction);

        if (!hookCtx[idx].pTrampoline)
        {
            continue;
        }

        ODPRINTF((L"mhooks: Mhook_UnhookEx: found struct at %p", hookCtx[idx].pTrampoline));

        hookCtx[idx].dwInstructionLength = hookCtx[idx].pTrampoline->cbOverwrittenCode;
    }

    VOID* procEnumerationCtx = NULL;
    PSYSTEM_PROCESS_INFORMATION procInfo = NULL;

    if (GetCurrentProcessSnapshot(&procEnumerationCtx, &procInfo))
    {
        // make sure nobody's executing code where we're about to overwrite a few bytes
        SuspendOtherThreads(hookCtx, hookCount, procInfo);

        for (int idx = 0; idx < hookCount; idx++)
        {
            if (!hookCtx[idx].pTrampoline)
            {
                continue;
            }

            DWORD dwOldProtectSystemFunction = 0;
            // make memory writable
            if (VirtualProtect(hookCtx[idx].pTrampoline->pSystemFunction, hookCtx[idx].pTrampoline->cbOverwrittenCode, PAGE_EXECUTE_READWRITE, &dwOldProtectSystemFunction))
            {
                ODPRINTF((L"mhooks: Mhook_UnhookEx: readwrite set on system function"));
                PBYTE pbCode = (PBYTE)hookCtx[idx].pTrampoline->pSystemFunction;
                for (DWORD i = 0; i < hookCtx[idx].pTrampoline->cbOverwrittenCode; i++)
                {
                    pbCode[i] = hookCtx[idx].pTrampoline->codeUntouched[i];
                }

                // flush instruction cache and make memory unwritable
                FlushInstructionCache(GetCurrentProcess(), hookCtx[idx].pTrampoline->pSystemFunction, hookCtx[idx].pTrampoline->cbOverwrittenCode);
                VirtualProtect(hookCtx[idx].pTrampoline->pSystemFunction, hookCtx[idx].pTrampoline->cbOverwrittenCode, dwOldProtectSystemFunction, &dwOldProtectSystemFunction);

                // return the original function pointer
                *hooks[idx] = hookCtx[idx].pTrampoline->pSystemFunction;
                result += 1;

                ODPRINTF((L"mhooks: Mhook_UnhookEx: sysfunc: %p", *hooks[idx]));

                // free the trampoline while not really discarding it from memory
                TrampolineFree(hookCtx[idx].pTrampoline, false);
                ODPRINTF((L"mhooks: Mhook_UnhookEx: unhook successful"));
            }
            else
            {
                ODPRINTF((L"mhooks: Mhook_UnhookEx: failed VirtualProtect 1: %d", gle()));
            }
        }

        // make the other guys runnable
        ResumeOtherThreads();

        CloseProcessSnapshot(procEnumerationCtx);
    }

    free(hookCtx);

    LeaveCritSec();

    return result;
}

//=========================================================================
BOOL Mhook_Unhook(PVOID *ppHookedFunction) 
{
    return Mhook_UnhookEx(&ppHookedFunction, 1) == 1;
}
