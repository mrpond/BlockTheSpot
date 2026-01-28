#pragma once
#include "loader.h"

void patch_instruction(LPVOID* lpAddress, void* value, SIZE_T patch_size);
