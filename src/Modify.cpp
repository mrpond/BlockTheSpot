#include "pch.h"

using _cef_urlrequest_create = void* (*)(void* request, void* client, void* request_context);
static _cef_urlrequest_create cef_urlrequest_create_orig = nullptr;

using _cef_string_userfree_utf16_free = void (*)(void* str);
static _cef_string_userfree_utf16_free cef_string_userfree_utf16_free_orig = nullptr;

using _cef_zip_reader_create = void* (*)(void* stream);
static _cef_zip_reader_create cef_zip_reader_create_orig = nullptr;

using _cef_zip_reader_t_read_file = int(__stdcall*)(void* self, void* buffer, size_t bufferSize);
static _cef_zip_reader_t_read_file cef_zip_reader_t_read_file_orig = nullptr;

#ifndef NDEBUG
void* cef_urlrequest_create_hook(struct _cef_request_t* request, void* client, void* request_context)
#else
void* cef_urlrequest_create_hook(void* request, void* client, void* request_context)
#endif
{
#ifndef NDEBUG
	cef_string_utf16_t* url_utf16 = request->get_url(request);
	std::wstring url = Utils::ToString(url_utf16->str);
#else
	const auto get_url = *(void* (__stdcall**)(void*))((uintptr_t)request + SettingsManager::m_cef_request_t_get_url_offset);
	auto url_utf16 = get_url(request);
	std::wstring url = *reinterpret_cast<wchar_t**>(url_utf16);
#endif
	for (const auto& block_url : SettingsManager::m_block_list) {
		if (std::wstring_view::npos != url.find(block_url)) {
			LogInfo(L"blocked - {}", url);
			cef_string_userfree_utf16_free_orig((void*)url_utf16);
			return nullptr;
		}
	}

	cef_string_userfree_utf16_free_orig((void*)url_utf16);
	LogInfo(L"allow - {}", url);
	return cef_urlrequest_create_orig(request, client, request_context);
}

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

DWORD WINAPI EnableDeveloper(LPVOID lpParam)
{
	auto& dev_data = SettingsManager::m_developer.at(SettingsManager::m_architecture);
	auto scan = MemoryScanner::ScanResult(dev_data.at(L"Address").get_integer(), L"", true);
	if (!scan.is_valid(dev_data.at(L"Signature").get_string())) {
		scan = MemoryScanner::ScanFirst(dev_data.at(L"Signature").get_string());
		dev_data.at(L"Address") = static_cast<int>(scan.rva());
	}

	if (scan.is_valid()) {
		if (scan.offset(dev_data.at(L"Offset").get_integer()).write(Utils::ToHexBytes(dev_data.at(L"Value").get_string()))) {
			LogInfo(L"Developer - successfully patched!");
		}
		else {
			LogError(L"Developer - failed to patch!");
		}
	}
	else {
		LogError(L"Developer - unable to find signature in memory!");
	}

	return 0;
}

DWORD WINAPI BlockAds(LPVOID lpParam)
{
	cef_string_userfree_utf16_free_orig = (_cef_string_userfree_utf16_free)MemoryScanner::GetFunctionAddress("libcef.dll", "cef_string_userfree_utf16_free").data();
	if (!cef_string_userfree_utf16_free_orig) {
		LogError(L"BlockAds - patch failed!");
		return 0;
	}

	cef_urlrequest_create_orig = (_cef_urlrequest_create)MemoryScanner::GetFunctionAddress("libcef.dll", "cef_urlrequest_create").hook((PVOID)cef_urlrequest_create_hook);
	cef_urlrequest_create_orig ? LogInfo(L"BlockAds - patch success!") : LogError(L"BlockAds - patch failed!");
	return 0;
}

DWORD WINAPI BlockBanner(LPVOID lpParam)
{
	cef_zip_reader_create_orig = (_cef_zip_reader_create)MemoryScanner::GetFunctionAddress("libcef.dll", "cef_zip_reader_create").hook((PVOID)cef_zip_reader_create_hook);
	cef_zip_reader_create_orig ? LogInfo(L"BlockBanner - patch success!") : LogError(L"BlockBanner - patch failed!");
	return 0;
}