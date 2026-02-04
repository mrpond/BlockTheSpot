#include "memory.h"

void patch_instruction(LPVOID* lpAddress, void* value, SIZE_T patch_size)
{
	DWORD oldProtect;
	VirtualProtect(lpAddress, patch_size, PAGE_EXECUTE_READWRITE, &oldProtect);
	memcpy(lpAddress, value, patch_size);
	VirtualProtect(lpAddress, patch_size, oldProtect, &oldProtect);
}
