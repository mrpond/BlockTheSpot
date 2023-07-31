#include "pch.h"

#ifndef NDEBUG

#include <intrin.h>

//typedef void (__stdcall* _cef_string_utf16_clear)(cef_string_utf16_t* str);
//_cef_string_utf16_clear cef_string_utf16_clear_orig = nullptr;
//
//void __stdcall cef_string_utf16_clear_hook(cef_string_utf16_t* str) {
//    try {
//        if (str->str) {
//            std::wstring wstr(*reinterpret_cast<wchar_t**>(str));
//            if (Utils::Contains(wstr, L"Spotify Free")) {
//                //reinterpret_cast<wchar_t*>(str) == L"Spotify";
//                //cef_string_utf16_set(L"Spotify", wcslen(L"Spotify"), str, 1);
//
//
//               // const wchar_t* newValue = L"Spotify";
//               // size_t newValueLength = wcslen(newValue);
//               // str->length = newValueLength;
//               // str->str = reinterpret_cast<wchar_t*>(realloc(str->str, (newValueLength + 1) * sizeof(wchar_t)));
//               // wmemcpy(str->str, newValue, newValueLength + 1);
//
//                Print({ Color::Yellow }, L"[{}]: {:#x} | {}", L"cef_string_utf16_clear", _ReturnAddress(), wstr);
//                //_wsystem(L"pause");
//            }
//        }
//    }
//    catch (const std::exception& e) {
//        Print({ Color::Red }, L"[{}] {}", L"ERROR", e.what());
//    }
//
//    cef_string_utf16_clear_orig(str);
//}


//typedef cef_zip_reader_t* (__stdcall* _cef_zip_reader_create)(cef_stream_reader_t* stream);
//_cef_zip_reader_create cef_zip_reader_create_orig = nullptr;
//
//typedef size_t(__stdcall* _cef_stream_reader_read)(cef_stream_reader_t* self, void* ptr, size_t size, size_t n);
//_cef_stream_reader_read cef_stream_reader_read_orig = nullptr;

//#ifndef NDEBUG
//using _cef_zip_reader_create = cef_zip_reader_t * (__stdcall*)(cef_stream_reader_t* stream);
//_cef_zip_reader_create cef_zip_reader_create_orig = nullptr;
//
//using _cef_stream_reader_read = size_t(__stdcall*)(cef_stream_reader_t* self, void* ptr, size_t size, size_t n);
//_cef_stream_reader_read cef_stream_reader_read_orig = nullptr;
//#else
//using _cef_zip_reader_create = void * (__stdcall*)(void* stream);
//_cef_zip_reader_create cef_zip_reader_create_orig = nullptr;
//
//using _cef_stream_reader_read = size_t(__stdcall*)(void* self, void* ptr, size_t size, size_t n);
//_cef_stream_reader_read cef_stream_reader_read_orig = nullptr;
//
//#endif
//
//
//std::map<cef_stream_reader_t*, std::wstring> streamReaderToFileMap;
//size_t __stdcall cef_stream_reader_read_hook(cef_stream_reader_t* self, void* ptr, size_t size, size_t n)
//{
//    size_t bytesRead = cef_stream_reader_read_orig(self, ptr, size, n);
//
//    if (bytesRead > 0) {
//        std::wstring wstrFileName = streamReaderToFileMap[self];
//        if (wstrFileName == L"index.html" || Utils::Contains(wstrFileName, L"download")) {
//            Print(L"{} {} {} {}", wstrFileName, size, n, (char*)ptr);
//        }
//    }
//
//    return bytesRead;
//}
//
//
//#ifndef NDEBUG
//cef_zip_reader_t* __stdcall cef_zip_reader_create_hook(cef_stream_reader_t* stream)
//{
//    cef_stream_reader_read_orig = stream->read;
//#else
//void* __stdcall cef_zip_reader_create_hook(void * stream)
//{
//    cef_stream_reader_read_orig = *(_cef_stream_reader_read*)((std::uintptr_t)stream + 40);
//#endif
//
//    Hooking::HookFunction(&(PVOID&)cef_stream_reader_read_orig, cef_stream_reader_read_hook);
//
//#ifndef NDEBUG
//    cef_zip_reader_t* zip_reader = cef_zip_reader_create_orig(stream);
//    while (zip_reader->move_to_next_file(zip_reader)) {
//        cef_string_userfree_t file_name = zip_reader->get_file_name(zip_reader);
//        std::wstring wstrFileName = file_name->str;
//#else
//    auto zip_reader = cef_zip_reader_create_orig(stream);
//    auto move_to_next_file = (*(unsigned int(__fastcall**)(void*))((std::uintptr_t)zip_reader + 48)); // orig = zip_reader
//    auto get_file_name = (*(void*(__fastcall**)(void*))((std::uintptr_t)zip_reader + 72));
//
//    while (move_to_next_file(zip_reader)) {
//        auto file_name = get_file_name(zip_reader);
//        std::wstring wstrFileName = (const wchar_t*)((__int64(__fastcall*)(void*))file_name)(zip_reader);
//        Print(L"{}", wstrFileName);
//#endif
//    //    
//    //    streamReaderToFileMap[stream] = wstrFileName;
//    //    cef_string_userfree_utf16_free(file_name);
//    }
//
//    return zip_reader;
//}

DWORD WINAPI Debug(LPVOID lpParam)
{
    try
    {
        Utils::MeasureExecutionTime([&]() {
            //Utils::PrintSymbols(L"chrome_elf.dll");
            //print_test();

            //const auto cef_string_utf16_clear_ptr = PatternScanner::GetFunctionAddress(L"libcef.dll", L"cef_string_utf16_clear");
            //cef_string_utf16_clear_orig = (_cef_string_utf16_clear)cef_string_utf16_clear_ptr.data();
            //Hooking::HookFunction(&(PVOID&)cef_string_utf16_clear_orig, cef_string_utf16_clear_hook);

            //const auto cef_zip_reader_create_ptr = PatternScanner::GetFunctionAddress(L"libcef.dll", L"cef_zip_reader_create");
            //cef_zip_reader_create_orig = (_cef_zip_reader_create)cef_zip_reader_create_ptr.data();
            //Hooking::HookFunction(&(PVOID&)cef_zip_reader_create_orig, cef_zip_reader_create_hook);

            }, L"DEBUG");
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
#endif