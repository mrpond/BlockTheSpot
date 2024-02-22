#pragma once
DWORD WINAPI EnableDeveloper(LPVOID lpParam);
DWORD WINAPI BlockAds(LPVOID lpParam);
DWORD WINAPI BlockBanner(LPVOID lpParam);

#ifdef _WIN64
static int cef_request_t_get_url_offset = 48;
static int cef_zip_reader_get_file_name_offset = 72;
static int cef_zip_reader_t_read_file_offset = 112;
#else
static int cef_request_t_get_url_offset = 24;
static int cef_zip_reader_get_file_name_offset = 36;
static int cef_zip_reader_t_read_file_offset = 56;
#endif