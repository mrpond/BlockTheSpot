#include "pch.h"

using _cef_urlrequest_create = void* (*)(void* request, void* client, void* request_context);
static _cef_urlrequest_create cef_urlrequest_create_orig = nullptr;

using _cef_string_userfree_utf16_free = void (*)(void* str);
static _cef_string_userfree_utf16_free cef_string_userfree_utf16_free_orig = nullptr;

using _cef_zip_reader_create = void* (*)(void* stream);
static _cef_zip_reader_create cef_zip_reader_create_orig = nullptr;

using _cef_zip_reader_t_read_file = int(__stdcall*)(void* self, void* buffer, size_t bufferSize);
static _cef_zip_reader_t_read_file cef_zip_reader_t_read_file_orig = nullptr;

static constexpr std::array<std::wstring_view, 3> block_list = { L"/ads/", L"/ad-logic/", L"/gabo-receiver-service/" };

#ifndef NDEBUG
void* cef_urlrequest_create_hook(struct _cef_request_t* request, void* client, void* request_context)
#else
void* cef_urlrequest_create_hook(void* request, void* client, void* request_context)
#endif
{
#ifndef NDEBUG
	cef_string_utf16_t* url_utf16 = request->get_url (request);
	std::wstring url = Utils::ToString(url_utf16->str);
#else
	const auto get_url = *(void* (__stdcall**)(void*))((uintptr_t)request + cef_request_t_get_url_offset);
	auto url_utf16 = get_url(request);
	std::wstring url = *reinterpret_cast<wchar_t**>(url_utf16);
#endif
	for (const auto& blockurl : block_list) {
		if (std::wstring_view::npos != url.find (blockurl)) {
			Log(L"blocked - " + url, LogLevel::Info);
			cef_string_userfree_utf16_free_orig((void*)url_utf16);
			return nullptr;
		}
	}
	cef_string_userfree_utf16_free_orig((void*)url_utf16);
	Log(L"allow - " + url, LogLevel::Info);
	return cef_urlrequest_create_orig (request, client, request_context);
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
	const auto get_file_name = (*(void* (__stdcall**)(void*))((uintptr_t)self + cef_zip_reader_get_file_name_offset));
	std::wstring file_name = *reinterpret_cast<wchar_t**>(get_file_name(self));
#endif

	if (file_name == L"home-hpto.css") {
		const auto hpto = MemoryScanner::ScanFirst(reinterpret_cast<uintptr_t>(buffer), bufferSize, L".WiPggcPDzbwGxoxwLWFf{display:-webkit-box;display:-ms-flexbox;display:flex;");
		if (hpto.is_valid()) {
			if (hpto.write(".WiPggcPDzbwGxoxwLWFf{display:-webkit-box;display:-ms-flexbox;display:none;")) {
				Log(L"hptocss patched!", LogLevel::Info);
			}
			else {
				Log(L"hptocss patch failed!", LogLevel::Error);
			}
		}
		else {
			Log(L"hptocss - failed not found!", LogLevel::Error);
		}
	}

	if (file_name == L"xpui.js") {
		const auto skipads = MemoryScanner::ScanFirst(reinterpret_cast<uintptr_t>(buffer), bufferSize, L"adsEnabled:!0");
		if (skipads.is_valid()) {
			if (skipads.offset(12).write("1")) {
				Log(L"adsEnabled patched!", LogLevel::Info);
			}
			else {
				Log(L"adsEnabled - patch failed!", LogLevel::Error);
			}
		}
		else {
			Log(L"adsEnabled - failed not found!", LogLevel::Error);
		}

		const auto sponsorship = MemoryScanner::ScanFirst(reinterpret_cast<uintptr_t>(buffer), bufferSize, L".set(\"allSponsorships\",t.sponsorships)}}(e,t);");
		if (sponsorship.is_valid()) {
			if (sponsorship.offset(5).write(std::string(15, ' ').append("\"").c_str())) {
				Log(L"sponsorship patched!", LogLevel::Info);
			}
			else {
				Log(L"sponsorship patch failed!", LogLevel::Error);
			}
		}
		else {
			Log(L"sponsorship - failed not found!", LogLevel::Error);
		}

		const auto skipsentry = MemoryScanner::ScanFirst(reinterpret_cast<uintptr_t>(buffer), bufferSize, L"sentry.io");
		if (skipsentry.is_valid()) {
			if (skipsentry.write("localhost")) {
				Log(L"sentry.io -> localhost patched!", LogLevel::Info);
			}
			else {
				Log(L"sentry.io -> localhost - patch failed!", LogLevel::Error);
			}
		}
		else {
			Log(L"sentry.io -> localhost - failed not found!", LogLevel::Error);
		}

		const auto ishptoenable = MemoryScanner::ScanFirst(reinterpret_cast<uintptr_t>(buffer), bufferSize, L"hptoEnabled:!0");
		if (ishptoenable.is_valid())
		{
			if (ishptoenable.offset(13).write("1")) {
				Log(L"hptoEnabled patched!", LogLevel::Info);
			}
			else {
				Log(L"hptoEnabled - patch failed!", LogLevel::Error);
			}
		}
		else {
			Log(L"hptoEnabled - failed not found!", LogLevel::Error);
		}

		const auto ishptohidden = MemoryScanner::ScanFirst(reinterpret_cast<uintptr_t>(buffer), bufferSize, L"isHptoHidden:!0");
		if (ishptohidden.is_valid()) {
			if (ishptohidden.offset(14).write("1")) {
				Log(L"isHptoHidden patched!", LogLevel::Info);
			}
			else {
				Log(L"isHptoHidden - patch failed!", LogLevel::Error);
			}
		}
		else {
			Log(L"isHptoHidden - failed not found!", LogLevel::Error);
		}

		const auto sp_localhost = MemoryScanner::ScanFirst(reinterpret_cast<uintptr_t>(buffer), bufferSize, L"sp://ads/v1/ads/");
		if (sp_localhost.is_valid()) {
			if (sp_localhost.write("sp://localhost//")) {
				Log(L"sp://ads/v1/ads/ patched!", LogLevel::Info);
			}
			else {
				Log(L"sp://ads/v1/ads/ - patch failed!", LogLevel::Error);
			}
		}
		else {
			Log(L"sp://ads/v1/ads/ - failed not found!", LogLevel::Error);
		}

		const auto premium_free = MemoryScanner::ScanFirst(reinterpret_cast<uintptr_t>(buffer), bufferSize, L"e.session?.productState?.catalogue?.toLowerCase()");
		if (premium_free.is_valid()) {
			//if (premium_free.offset(-1).write(std::string(48, ' ').append("\"\""))) {
			if (premium_free.write("\"blockthespot-team-says-meow-meow-meow-meow-meow\"")) {
				Log(L"premium patched!", LogLevel::Info);
			}
			else {
				Log(L"premium - patch failed!", LogLevel::Error);
			}
		}
		else {
			Log(L"premium - failed not found!", LogLevel::Error);
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
	cef_zip_reader_t_read_file_orig = *(_cef_zip_reader_t_read_file*)((uintptr_t)zip_reader + cef_zip_reader_t_read_file_offset);
#endif

	if (!Hooking::HookFunction(&(PVOID&)cef_zip_reader_t_read_file_orig, (PVOID)cef_zip_reader_t_read_file_hook)) {
		Log(L"zip_reader_read_file_hook - patch failed!", LogLevel::Error);
	}
	
	return zip_reader;
}

DWORD WINAPI EnableDeveloper(LPVOID lpParam)
{
#ifdef _WIN64
	const auto app_developer = MemoryScanner::ScanFirst(L"app-developer").get_all_matching_codes({ 0x48, 0x8D, 0x15 });	
	const auto developer = app_developer.size() > 1 ? app_developer[1].scan_first(L"D1 EB").offset(2) : MemoryScanner::ScanResult();
	if (developer.is_valid({ 0x80, 0xE3, 0x01 })) {
		if (developer.write({ 0xB3, 0x01, 0x90 })) {
			Log(L"Developer - patch success!", LogLevel::Info);
		}
		else {
			Log(L"Developer - patch failed!", LogLevel::Error);
		}
	}
	else {
		Log(L"Developer - failed not found!", LogLevel::Error);
	}
#else
	//const auto app_developer = MemoryScanner::ScanFirst(L"app-developer").get_all_matching_codes({ 0x68 }, false);
	const auto developer = MemoryScanner::ScanFirst(L"25 01 FF FF FF 89 ?? ?? ?? FF FF");
	if (developer.is_valid()) {
		if (developer.write({ 0xB8, 0x03, 0x00 })) {
			Log(L"Developer - patch success!", LogLevel::Info);
		}
		else {
			Log(L"Developer - patch failed!", LogLevel::Error);
		}
	}
	else {
		Log(L"Developer - failed not found!", LogLevel::Error);
	}
#endif
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