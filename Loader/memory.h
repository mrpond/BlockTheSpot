#pragma once
#include "loader.h"

void patch_instruction(LPVOID* lpAddress, SIZE_T hex_size, void* value);
