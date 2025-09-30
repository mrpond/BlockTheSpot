#pragma once
#include <windows.h>
#include <cstddef>

#ifndef NDEBUG
#include <include/capi/cef_urlrequest_capi.h>
#include <include/capi/cef_zip_reader_capi.h>

using cef_string_userfree_utf16_free_t = decltype(&cef_string_userfree_utf16_free);
using cef_urlrequest_create_t = decltype(&cef_urlrequest_create);
using cef_zip_reader_create_t = decltype(&cef_zip_reader_create);
using cef_request_get_url_t = decltype(cef_request_t::get_url);
using cef_zip_reader_read_file_t = decltype(cef_zip_reader_t::read_file);
using cef_zip_reader_get_file_name_t = decltype(cef_zip_reader_t::get_file_name);
using cef_zip_reader_get_file_size_t = decltype(cef_zip_reader_t::get_file_size);
using cef_zip_reader_open_file_t = decltype(cef_zip_reader_t::open_file);
using cef_zip_reader_close_file_t = decltype(cef_zip_reader_t::close_file);

#else
typedef struct {
    char16_t* str;
    size_t length;
} cef_string_utf16_t;

using cef_string_userfree_utf16_t = cef_string_utf16_t*;
using cef_urlrequest_t = void;
using _cef_request_t = void;
using _cef_urlrequest_client_t = void;
using _cef_request_context_t = void;
using cef_zip_reader_t = void;
using _cef_zip_reader_t = void;
using _cef_stream_reader_t = void;

using cef_string_userfree_utf16_free_t = void(*)(cef_string_userfree_utf16_t str);
using cef_urlrequest_create_t = cef_urlrequest_t*(*)(_cef_request_t* request, _cef_urlrequest_client_t* client, _cef_request_context_t* request_context);
using cef_zip_reader_create_t = cef_zip_reader_t*(*)(_cef_stream_reader_t* stream);
using cef_request_get_url_t = cef_string_userfree_utf16_t(__stdcall*)(_cef_request_t* self);
using cef_zip_reader_read_file_t = int(__stdcall*)(_cef_zip_reader_t* self, void* buffer, size_t bufferSize);
using cef_zip_reader_get_file_name_t = cef_string_userfree_utf16_t(__stdcall*)(_cef_zip_reader_t* self);
using cef_zip_reader_get_file_size_t = int64_t(__stdcall*)(_cef_zip_reader_t* self);
using cef_zip_reader_open_file_t = int(__stdcall*)(_cef_zip_reader_t* self, const cef_string_utf16_t* password);
using cef_zip_reader_close_file_t = int(__stdcall*)(_cef_zip_reader_t* self);
#endif

DWORD WINAPI AdsBlock(LPVOID lpParam);
DWORD WINAPI BannerBlock(LPVOID lpParam);
DWORD WINAPI DevMode(LPVOID lpParam);

cef_urlrequest_t* cef_urlrequest_create_hook(_cef_request_t* request, _cef_urlrequest_client_t* client, _cef_request_context_t* request_context);
cef_zip_reader_t* cef_zip_reader_create_hook(_cef_stream_reader_t* stream);
int __stdcall cef_zip_reader_read_file_hook(_cef_zip_reader_t* self, void* buffer, size_t bufferSize);
int64_t __stdcall cef_zip_reader_get_file_size_hook(_cef_zip_reader_t* self);

static void free_cef_string(cef_string_userfree_utf16_t s);
static std::wstring get_file_name(_cef_zip_reader_t* self);
static bool open_file(_cef_zip_reader_t* self, const cef_string_utf16_t* password);
static void close_file(_cef_zip_reader_t* self);
static int read_file(_cef_zip_reader_t* self, void* buffer, size_t bufferSize);

static cef_urlrequest_create_t cef_urlrequest_create_orig = nullptr;
static cef_zip_reader_create_t cef_zip_reader_create_orig = nullptr;
static cef_request_get_url_t cef_request_get_url_orig = nullptr;
static cef_zip_reader_read_file_t cef_zip_reader_read_file_orig = nullptr;
static cef_zip_reader_get_file_name_t cef_zip_reader_get_file_name_orig = nullptr;
static cef_zip_reader_get_file_size_t cef_zip_reader_get_file_size_orig = nullptr;
static cef_zip_reader_open_file_t cef_zip_reader_open_file_orig = nullptr;
static cef_zip_reader_close_file_t cef_zip_reader_close_file_orig = nullptr;
