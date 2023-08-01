#include "pch.h"

using _cef_urlrequest_create = void* (*)(void* request, void* client, void* request_context);
static _cef_urlrequest_create cef_urlrequest_create_orig = nullptr;

using _cef_string_userfree_utf16_free = void (*)(void* str);
static _cef_string_userfree_utf16_free cef_string_userfree_utf16_free_orig = nullptr;

using _cef_zip_reader_create = void* (*)(void* stream);
static _cef_zip_reader_create cef_zip_reader_create_orig = nullptr;

using _cef_zip_reader_read_file = int(__stdcall*)(void* self, void* buffer, size_t bufferSize);
static _cef_zip_reader_read_file cef_zip_reader_read_file_orig = nullptr;

static constexpr std::array<std::wstring_view, 3> block_list = { L"/ads/", L"/ad-logic/", L"/gabo-receiver-service/" };

#ifndef NDEBUG
void* cef_urlrequest_create_hook(struct _cef_request_t* request, void* client, void* request_context)
#else
void* cef_urlrequest_create_hook(void* request, void* client, void* request_context)
#endif
{
#ifndef NDEBUG
	cef_string_utf16_t* url_utf16 = request->get_url (request);
	std::wstring url(Utils::ToString(url_utf16->str));
	//Print({ Color::Yellow }, L"[{}] {}", L"request_get_url", Memory::GetMemberFunctionOffset(&_cef_request_t::get_url));
#else

#ifdef _WIN64
	auto request_get_url = *(void* (__stdcall**)(void*))((std::uintptr_t)request + 48);
	auto url_utf16 = request_get_url(request);
	std::wstring url(*reinterpret_cast<wchar_t**>(url_utf16));
#else
	auto request_get_url = *(void* (__stdcall**)(void*))((std::uintptr_t)request + 24);
	auto url_utf16 = request_get_url(request);
	std::wstring url(*reinterpret_cast<wchar_t**>(url_utf16));
#endif
	
#endif
	for (const auto& blockurl : block_list) {
		if (std::wstring_view::npos != url.find (blockurl)) {
			Logger::Log(L"blocked - " + url, Logger::LogLevel::Info);
			cef_string_userfree_utf16_free_orig((void*)url_utf16);
			return nullptr;
		}
	}
	cef_string_userfree_utf16_free_orig((void*)url_utf16);
	Logger::Log(L"allow - " + url, Logger::LogLevel::Info);
	return cef_urlrequest_create_orig (request, client, request_context);
}

#ifndef NDEBUG
int cef_zip_reader_read_file_hook(struct _cef_zip_reader_t* self, void* buffer, size_t bufferSize)
#else
int cef_zip_reader_read_file_hook(void* self, void* buffer, size_t bufferSize)
#endif
{
	int _retval = cef_zip_reader_read_file_orig(self, buffer, bufferSize);
	try {
#ifndef NDEBUG
		std::wstring file_name = Utils::ToString(self->get_file_name(self)->str);
		//Print({ Color::Yellow }, L"[{}] {}", L"zip_reader_read_file", Memory::GetMemberFunctionOffset(&_cef_zip_reader_t::get_file_name));
#else
#ifdef _WIN64
		auto get_file_name = (*(void* (__stdcall**)(void*))((std::uintptr_t)self + 72));
#else
		auto get_file_name = (*(void* (__stdcall**)(void*))((std::uintptr_t)self + 36));
#endif
		std::wstring file_name(*reinterpret_cast<wchar_t**>(get_file_name(self)));
#endif

		//Logger::Log(Utils::FormatString(L"{} {} {:X}", file_name, bufferSize, reinterpret_cast<std::size_t>(buffer)), Logger::LogLevel::Info);
		if (file_name == L"home-hpto.css")
		{
			//Print(L"{} {} {:X}", file_name, bufferSize, buffer);
			//_wsystem(L"pause");
			const auto hpto = PatternScanner::ScanFirst(reinterpret_cast<std::size_t>(buffer), bufferSize, L".WiPggcPDzbwGxoxwLWFf{-webkit-box-pack:center;-ms-flex-pack:center;display:-webkit-box;display:-ms-flexbox;display:flex;");
			if (hpto.is_found()) {
				if (Memory::Write<const char*>(hpto.data(), ".WiPggcPDzbwGxoxwLWFf{-webkit-box-pack:center;-ms-flex-pack:center;display:-webkit-box;display:-ms-flexbox;display:none;")) {
					Logger::Log(L"hptocss patched!", Logger::LogLevel::Info);
				}
				else {
					Logger::Log(L"hptocss patch failed!", Logger::LogLevel::Error);
				}
			}
			else {
				Logger::Log(L"hptocss - failed not found!", Logger::LogLevel::Error);
			}
		}

		if (file_name == L"xpui-routes-profile.js")
		{
			//Print(L"{} {} {:X}", file_name, bufferSize, buffer);
			const auto isModalOpen = PatternScanner::ScanAll(reinterpret_cast<std::size_t>(buffer), bufferSize, L"isModalOpen:!0");
			if (isModalOpen[0].is_found()) {
				for (const auto& it : isModalOpen) {
					if (Memory::Write<const char>(it.offset(13).data(), '1')) {
						Logger::Log(L"isModalOpen patched!", Logger::LogLevel::Info);
					}
					else {
						Logger::Log(L"isModalOpen - patch failed!", Logger::LogLevel::Error);
					}
				}
			}
			else {
				Logger::Log(L"isModalOpen - failed not found!", Logger::LogLevel::Error);
			}
		}

		if (file_name == L"xpui.js")
		{
			//Print(L"{} {} {:X}", file_name, bufferSize, buffer);
			const auto skipads = PatternScanner::ScanFirst(reinterpret_cast<std::size_t>(buffer), bufferSize, L"adsEnabled:!0");
			if (skipads.is_found()) {
				if (Memory::Write<const char>(skipads.offset(12).data(), '1')) {
					Logger::Log(L"adsEnabled patched!", Logger::LogLevel::Info);
				}
				else {
					Logger::Log(L"adsEnabled - patch failed!", Logger::LogLevel::Error);
				}
			}
			else {
				Logger::Log(L"adsEnabled - failed not found!", Logger::LogLevel::Error);
			}

			const auto sponsorship = PatternScanner::ScanFirst(reinterpret_cast<std::size_t>(buffer), bufferSize, L".set(\"allSponsorships\",t.sponsorships)}}(e,t);");
			if (sponsorship.is_found())
			{
				memset((char*)sponsorship.data() + 6, 0x22, 1);
				memset((char*)sponsorship.data() + 7, 0x20, 15);
				Logger::Log(L"sponsorship patched!", Logger::LogLevel::Info);
				//if (Memory::Write<char>(sponsorship.offset(6).data(), ' ', 15)) {
				//	Logger::Log(L"sponsorship patched!", Logger::LogLevel::Info);
				//}
				//else {
				//	Logger::Log(L"sponsorship patch failed!", Logger::LogLevel::Error);
				//}
			}
			else {
				Logger::Log(L"sponsorship - failed not found!", Logger::LogLevel::Error);
			}

			const auto skipsentry = PatternScanner::ScanFirst(reinterpret_cast<std::size_t>(buffer), bufferSize, L"sentry.io");
			if (skipsentry.is_found()) {
				if (Memory::Write<const char*>(skipsentry.data(), "localhost")) {
					Logger::Log(L"sentry.io -> localhost patched!", Logger::LogLevel::Info);
				}
				else {
					Logger::Log(L"sentry.io -> localhost - patch failed!", Logger::LogLevel::Error);
				}
			}
			else {
				Logger::Log(L"sentry.io -> localhost - failed not found!", Logger::LogLevel::Error);
			}

			const auto ishptoenable = PatternScanner::ScanFirst(reinterpret_cast<std::size_t>(buffer), bufferSize, L"hptoEnabled:!0");
			if (ishptoenable.is_found())
			{
				if (Memory::Write<const char>(ishptoenable.offset(13).data(), '1')) {
					Logger::Log(L"hptoEnabled patched!", Logger::LogLevel::Info);
				}
				else {
					Logger::Log(L"hptoEnabled - patch failed!", Logger::LogLevel::Error);
				}
			}
			else {
				Logger::Log(L"hptoEnabled - failed not found!", Logger::LogLevel::Error);
			}

			const auto ishptohidden = PatternScanner::ScanFirst(reinterpret_cast<std::size_t>(buffer), bufferSize, L"isHptoHidden:!0");
			if (ishptohidden.is_found()) {
				if (Memory::Write<const char>(ishptohidden.offset(14).data(), '1')) {
					Logger::Log(L"isHptoHidden patched!", Logger::LogLevel::Info);
				}
				else {
					Logger::Log(L"isHptoHidden - patch failed!", Logger::LogLevel::Error);
				}
			}
			else {
				Logger::Log(L"isHptoHidden - failed not found!", Logger::LogLevel::Error);
			}

			const auto sp_localhost = PatternScanner::ScanFirst(reinterpret_cast<std::size_t>(buffer), bufferSize, L"sp://ads/v1/ads/");
			if (sp_localhost.is_found()) {
				if (Memory::Write<const char*>(sp_localhost.data(), "sp://localhost//")) {
					Logger::Log(L"sp://ads/v1/ads/ patched!", Logger::LogLevel::Info);
				}
				else {
					Logger::Log(L"sp://ads/v1/ads/ - patch failed!", Logger::LogLevel::Error);
				}
			}
			else {
				Logger::Log(L"sp://ads/v1/ads/ - failed not found!", Logger::LogLevel::Error);
			}

			const auto premium_free = PatternScanner::ScanFirst(reinterpret_cast<std::size_t>(buffer), bufferSize, L"\"free\"===e.session?.productState?.catalogue?.toLowerCase(),r=e=>null!==e.session?.productState&&1===parseInt(e.session?.productState?.ads,10),o=e=>\"premium\"===e.session?.productState?.catalogue?.toLowerCase(),");
			if (premium_free.is_found()) {
				//Print(L"{}", premium_free.read<const char*>());
				//system("pause");
				if (Memory::Write<const char*>(premium_free.data(), "\"premium\"===e.session?.productState?.catalogue?.toLowerCase(),r=e=>null!==e.session?.productState&&1===parseInt(e.session?.productState?.ads,10),o=e=>\"free\"===e.session?.productState?.catalogue?.toLowerCase(),")) {
					Logger::Log(L"premium patched!", Logger::LogLevel::Info);
				}
				else {
					Logger::Log(L"premium - patch failed!", Logger::LogLevel::Error);
				}
			}
			else {
				Logger::Log(L"premium - failed not found!", Logger::LogLevel::Error);
			}
		}
	}
	catch (const std::exception& e) {
		Print({ Color::Red }, L"[{}] {}", L"ERROR", e.what());
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
	cef_zip_reader_read_file_orig = (_cef_zip_reader_read_file)zip_reader->read_file;
	//Print({ Color::Yellow }, L"[{}] {}", L"zip_reader_read_file", Memory::GetMemberFunctionOffset(&cef_zip_reader_t::read_file));

#else
	auto zip_reader = cef_zip_reader_create_orig(stream);

#ifdef _WIN64
	cef_zip_reader_read_file_orig = *(_cef_zip_reader_read_file*)((std::uintptr_t)zip_reader + 112);
#else
	cef_zip_reader_read_file_orig = *(_cef_zip_reader_read_file*)((std::uintptr_t)zip_reader + 56);
#endif

#endif

	if (cef_zip_reader_read_file_orig) {
		Hooking::HookFunction(&(PVOID&)cef_zip_reader_read_file_orig, cef_zip_reader_read_file_hook);
	}

	return zip_reader;
}

DWORD WINAPI EnableDeveloper(LPVOID lpParam)
{
	try
	{
#ifdef _WIN64
		const auto developer = PatternScanner::ScanFirst(L"41 22 DE 48 8B 95 40 05 00 00");
		if (developer.is_found()) {
			if (Memory::Write<std::vector<std::uint8_t>>(developer.data(), { 0xB3, 0x03, 0x90 })) {
				Logger::Log(L"Developer - patch success!", Logger::LogLevel::Info);
			}
			else {
				Logger::Log(L"Developer - patch failed!", Logger::LogLevel::Error);
			}
		}
		else {
			Logger::Log(L"Developer - failed not found!", Logger::LogLevel::Error);
		}
#else
		const auto developer = PatternScanner::ScanFirst(L"25 01 FF FF FF 89 ?? ?? ?? FF FF");
		if (developer.is_found()) {
			if (Memory::Write<std::vector<std::uint8_t>>(developer.data(), { 0xB8, 0x03, 0x00 })) {
				Logger::Log(L"Developer - patch success!", Logger::LogLevel::Info);
			}
			else {
				Logger::Log(L"Developer - patch failed!", Logger::LogLevel::Error);
			}
		}
		else {
			Logger::Log(L"Developer - failed not found!", Logger::LogLevel::Error);
		}
#endif
	}
	catch (const std::exception& e)
	{
		Print({ Color::Red }, L"[{}] {}", L"ERROR", e.what());
	}
	return 0;
}

DWORD WINAPI BlockAds(LPVOID lpParam)
{
	try
	{
		cef_urlrequest_create_orig = (_cef_urlrequest_create)PatternScanner::GetFunctionAddress(L"libcef.dll", L"cef_urlrequest_create").data();
		cef_string_userfree_utf16_free_orig = (_cef_string_userfree_utf16_free)PatternScanner::GetFunctionAddress(L"libcef.dll", L"cef_string_userfree_utf16_free").data();

		if (cef_urlrequest_create_orig && cef_string_userfree_utf16_free_orig) {
			if (Hooking::HookFunction(&(PVOID&)cef_urlrequest_create_orig, (PVOID)cef_urlrequest_create_hook)) {
				Logger::Log(L"BlockAds - patch success!", Logger::LogLevel::Info);
			}
			else {
				Logger::Log(L"BlockAds - patch failed!", Logger::LogLevel::Error);
			}
		}
	}
	catch (const std::exception& e)
	{
		Print({ Color::Red }, L"[{}] {}", L"ERROR", e.what());
	}
	return 0;
}

DWORD WINAPI BlockBanner(LPVOID lpParam)
{
	try
	{
		cef_zip_reader_create_orig = (_cef_zip_reader_create)PatternScanner::GetFunctionAddress(L"libcef.dll", L"cef_zip_reader_create").data();

		if (cef_zip_reader_create_orig) {
			if (Hooking::HookFunction(&(PVOID&)cef_zip_reader_create_orig, cef_zip_reader_create_hook)) {
				Logger::Log(L"BlockBanner - patch success!", Logger::LogLevel::Info);
			}
			else {
				Logger::Log(L"BlockBanner - patch failed!", Logger::LogLevel::Error);
			}
		}
	}
	catch (const std::exception& e)
	{
		Print({ Color::Red }, L"[{}] {}", L"ERROR", e.what());
	}
	return 0;
}