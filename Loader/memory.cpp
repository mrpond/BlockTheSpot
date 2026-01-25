#include "memory.h"

void patch_instruction(LPVOID* lpAddress,SIZE_T size, void* value)
{
	DWORD oldProtect;
	VirtualProtect(lpAddress, size, PAGE_EXECUTE_READWRITE, &oldProtect);
	memcpy(lpAddress, value, size);
	VirtualProtect(lpAddress, size, oldProtect, &oldProtect);
}
