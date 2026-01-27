#include "pch.h"

using _cef_zip_reader_create = void* (*)(void* stream);
static _cef_zip_reader_create cef_zip_reader_create_orig = nullptr;

using _cef_zip_reader_t_read_file = int(__stdcall*)(void* self, void* buffer, size_t bufferSize);
static _cef_zip_reader_t_read_file cef_zip_reader_t_read_file_orig = nullptr;

#ifndef NDEBUG
int cef_zip_reader_t_read_file_hook(struct _cef_zip_reader_t* self, void* buffer, size_t bufferSize)
#else
int cef_zip_reader_t_read_file_hook(void* self, void* buffer, size_t bufferSize)
#endif
{
	int _retval = cef_zip_reader_t_read_file_orig(self, buffer, bufferSize);

#ifndef NDEBUG
	std::wstring file_name = Utils::ToString(self->get_file_name(self)->str);
#else
	const auto get_file_name = (*(void* (__stdcall**)(void*))((uintptr_t)self + SettingsManager::m_cef_zip_reader_t_get_file_name_offset));
	std::wstring file_name = *reinterpret_cast<wchar_t**>(get_file_name(self));
#endif

	if (SettingsManager::m_zip_reader.contains(file_name)) {
		for (auto& [setting_name, setting_data] : SettingsManager::m_zip_reader.at(file_name)) {
			auto scan = MemoryScanner::ScanResult(setting_data.at(L"Address").get_integer(), reinterpret_cast<uintptr_t>(buffer), bufferSize, true);
			if (!scan.is_valid(setting_data.at(L"Signature").get_string())) {
				scan = MemoryScanner::ScanFirst(reinterpret_cast<uintptr_t>(buffer), bufferSize, setting_data.at(L"Signature").get_string());
				setting_data.at(L"Address") = static_cast<int>(scan.rva());
			}

			if (scan.is_valid()) {
				const std::wstring fill_value(setting_data.at(L"Fill").get_integer(), L' ');
				if (scan.offset(setting_data.at(L"Offset").get_integer()).write(Utils::ToString(fill_value + setting_data.at(L"Value").get_string()))) {
					LogInfo(L"{} - patch success!", setting_name);
				}
				else {
					LogError(L"{} - patch failed!", setting_name);
				}
			}
			else {
				LogError(L"{} - unable to find signature in memory!", setting_name);
			}
		}
	}
	return _retval;
}

#ifndef NDEBUG
cef_zip_reader_t* cef_zip_reader_create_hook(cef_stream_reader_t* stream)
#else
void* cef_zip_reader_create_hook(void* stream)
#endif
{
#ifndef NDEBUG
	cef_zip_reader_t* zip_reader = (cef_zip_reader_t*)cef_zip_reader_create_orig(stream);
	cef_zip_reader_t_read_file_orig = (_cef_zip_reader_t_read_file)zip_reader->read_file;
#else
	auto zip_reader = cef_zip_reader_create_orig(stream);
	cef_zip_reader_t_read_file_orig = *(_cef_zip_reader_t_read_file*)((uintptr_t)zip_reader + SettingsManager::m_cef_zip_reader_t_read_file_offset);
#endif

	if (!Hooking::HookFunction(&(PVOID&)cef_zip_reader_t_read_file_orig, (PVOID)cef_zip_reader_t_read_file_hook)) {
		LogError(L"Failed to hook cef_zip_reader::read_file function!");
	}
	else {
		Hooking::UnhookFunction(&(PVOID&)cef_zip_reader_create_orig);
	}

	return zip_reader;
}


DWORD WINAPI BlockBanner(LPVOID lpParam)
{
	cef_zip_reader_create_orig = (_cef_zip_reader_create)MemoryScanner::GetFunctionAddress("libcef.dll", "cef_zip_reader_create").hook((PVOID)cef_zip_reader_create_hook);
	cef_zip_reader_create_orig ? LogInfo(L"BlockBanner - patch success!") : LogError(L"BlockBanner - patch failed!");
	return 0;
}