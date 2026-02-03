#pragma once
#include "loader.h"

void hook_cef_url(HMODULE libcef_dll_handle) noexcept;
void* cef_urlrequest_create_stub(void* request, void* client, void* request_context);