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
			Log(L"blocked - " + url, LogLevel::Info);
			cef_string_userfree_utf16_free_orig((void*)url_utf16);
			return nullptr;
		}
	}

	cef_string_userfree_utf16_free_orig((void*)url_utf16);
	Log(L"allow - " + url, LogLevel::Info);
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
		for (auto& [name, data] : SettingsManager::m_zip_reader.at(file_name)) {
			const auto& sig = data.at(L"Signature").get_string();
			auto scan = MemoryScanner::ScanResult(data.at(L"Address").get_integer(), reinterpret_cast<uintptr_t>(buffer), bufferSize, true);
			if (!scan.is_valid(sig)) {
				scan = MemoryScanner::ScanFirst(reinterpret_cast<uintptr_t>(buffer), bufferSize, sig);
				data.at(L"Address") = static_cast<int>(scan.rva());
			}

			if (scan.is_valid()) {
				const auto& value = data.at(L"Value").get_string();
				const auto& offset = data.at(L"Offset").get_integer();
				const auto& fill = data.at(L"Fill").get_integer();

				if (fill > 0) {
					scan.offset(offset).write(Utils::ToString(std::wstring(fill, ' ').append(value))) ? Log(name + L" - patch success!", LogLevel::Info) : Log(name + L" - patch failed!", LogLevel::Error);
				}
				else {
					scan.offset(offset).write(Utils::ToString(value)) ? Log(name + L" - patch success!", LogLevel::Info) : Log(name + L" - patch failed!", LogLevel::Error);
				}
			}
			else {
				Log(name + L" - unable to find signature in memory!", LogLevel::Error);
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
		Log(L"Failed to hook cef_zip_reader::read_file function!", LogLevel::Error);
	}
	else {
		Hooking::UnhookFunction(&(PVOID&)cef_zip_reader_create_orig);
	}

	return zip_reader;
}

DWORD WINAPI EnableDeveloper(LPVOID lpParam)
{
	auto& dev_data = SettingsManager::m_developer.at(SettingsManager::m_architecture);
	const auto& sig = dev_data.at(L"Signature").get_string();
	auto scan = MemoryScanner::ScanResult(dev_data.at(L"Address").get_integer(), L"", true);
	if (!scan.is_valid(sig)) {
		scan = MemoryScanner::ScanFirst(sig);
		dev_data.at(L"Address") = static_cast<int>(scan.rva());
	}

	if (scan.is_valid()) {
		if (scan.offset(dev_data.at(L"Offset").get_integer()).write(Utils::ToHexBytes(dev_data.at(L"Value").get_string()))) {
			Log(L"Developer - successfully patched!", LogLevel::Info);
		}
		else {
			Log(L"Developer - failed to patch!", LogLevel::Error);
		}
	}
	else {
		Log(L"Developer - unable to find signature in memory!", LogLevel::Error);
	}

	return 0;
}

DWORD WINAPI BlockAds(LPVOID lpParam)
{
	cef_string_userfree_utf16_free_orig = (_cef_string_userfree_utf16_free)MemoryScanner::GetFunctionAddress("libcef.dll", "cef_string_userfree_utf16_free").data();
	if (!cef_string_userfree_utf16_free_orig) {
		Log(L"BlockAds - patch failed!", LogLevel::Error);
		return 0;
	}

	cef_urlrequest_create_orig = (_cef_urlrequest_create)MemoryScanner::GetFunctionAddress("libcef.dll", "cef_urlrequest_create").hook((PVOID)cef_urlrequest_create_hook);
	cef_urlrequest_create_orig ? Log(L"BlockAds - patch success!", LogLevel::Info) : Log(L"BlockAds - patch failed!", LogLevel::Error);
	return 0;
}

DWORD WINAPI BlockBanner(LPVOID lpParam)
{
	cef_zip_reader_create_orig = (_cef_zip_reader_create)MemoryScanner::GetFunctionAddress("libcef.dll", "cef_zip_reader_create").hook((PVOID)cef_zip_reader_create_hook);
	cef_zip_reader_create_orig ? Log(L"BlockBanner - patch success!", LogLevel::Info) : Log(L"BlockBanner - patch failed!", LogLevel::Error);
	return 0;
}