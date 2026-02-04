#pragma once
#include "loader.h"

void hook_cef_reader(HMODULE libcef_dll_handle) noexcept;
void* cef_zip_reader_create_stub(void* stream);