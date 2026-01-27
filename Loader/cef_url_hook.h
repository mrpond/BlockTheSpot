#pragma once
#include "loader.h"

inline char cef_block_list[MAX_CEF_BLOCK_LIST][MAX_URL_LEN] = {};

using cef_urlrequest_create_t = void* (*)(void* request, void* client, void* request_context);
inline cef_urlrequest_create_t cef_urlrequest_create_orig = nullptr;

using cef_string_userfree_utf16_free_t = void (*)(void* str);
inline cef_string_userfree_utf16_free_t cef_string_userfree_utf16_free_orig = nullptr;

#ifdef USE_LIBCEF
void* cef_urlrequest_create_hook(struct _cef_request_t* request, void* client, void* request_context);
#else
void* cef_urlrequest_create_hook(void* request, void* client, void* request_context);
#endif

void hook_cef_url() noexcept;
