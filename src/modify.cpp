#include <iostream>
#include <regex>
#include "BasicUtils/memory.hpp"
#include "BasicUtils/logger.hpp"
#include "BasicUtils/utils.hpp"
#include "BasicUtils/json.hpp"
#include "modify.h"
#include "config_manager.h"

#define USE_PATCHED_FILE_CACHE

using namespace utils;
using CMgr = ConfigManager;

DWORD WINAPI AdsBlock(LPVOID lpParam) {
    if (auto addr = MemoryW(L"libcef.dll").GetFunction("cef_urlrequest_create")) {
        cef_urlrequest_create_orig = addr.Call<cef_urlrequest_create_t>();
        if (addr.Hook(&cef_urlrequest_create_orig, cef_urlrequest_create_hook)) {
            WLOG_INFO(L"[AdsBlock] Hooked 'cef_urlrequest_create' successfully.");
        } else {
            WLOG_ERROR(L"[AdsBlock] Failed to hook 'cef_urlrequest_create'.");
        }
    } else {
        WLOG_WARN(L"[AdsBlock] Function 'cef_urlrequest_create' not found.");
    }

    return 0;
}

DWORD WINAPI BannerBlock(LPVOID lpParam) {
    if (auto addr = MemoryW(L"libcef.dll").GetFunction("cef_zip_reader_create")) {
        cef_zip_reader_create_orig = addr.Call<cef_zip_reader_create_t>();
        if (addr.Hook(&cef_zip_reader_create_orig, cef_zip_reader_create_hook)) {
            WLOG_INFO(L"[BannerBlock] Hooked 'cef_zip_reader_create' successfully.");
        } else {
            WLOG_ERROR(L"[BannerBlock] Failed to hook 'cef_zip_reader_create'.");
        }
    } else {
        WLOG_WARN(L"[BannerBlock] Function 'cef_zip_reader_create' not found.");
    }

    return 0;
}

DWORD WINAPI DevMode(LPVOID){
    auto& p = CMgr::dev_mode_patch; auto m = MemoryW(L"spotify.dll");
    auto matches = [&]{
        std::vector<MemoryW::Address> v;
        std::erase_if(p.rvas, [&](auto r){
            if (auto a = m.AtAddress(r, p.pattern, true)) { v.push_back(a); return false; }
            return true;
        });
        return m.Scan(p.pattern, !p.replace_all);
    }();
    
    if (matches.empty()) return (WLOG_ERROR(L"[DevMode] Patch skipped (not found)."), 0);
    bool any = false;
    for (auto a : matches) {
        if(a.Offset(p.offset).Write(HexUtilsW::FromHexToBytes<std::vector<uint8_t>>(p.value))) {
            auto r = a.RVA();
            if(std::find(p.rvas.begin(), p.rvas.end(), r) == p.rvas.end()) { any = true; p.rvas.push_back(r); }
            WLOG_INFO(L"[DevMode] '{}' patched at {:#X}", p.pattern.c_str(), r);
        } else WLOG_ERROR(L"[DevMode] write failed at {:#X}", a.RVA());
    }
    if (any) CMgr::FlushToJson();
    return 0;
}

cef_urlrequest_t* cef_urlrequest_create_hook(_cef_request_t* request, _cef_urlrequest_client_t* client, _cef_request_context_t* request_context)
{
#ifndef NDEBUG
    auto u = request->get_url(request);
#else
    if (!cef_request_get_url_orig) {
        if (auto addr = MemoryW(L"libcef.dll").AtAddress(request).Resolve(CMgr::get_url_index * sizeof(void*))) {
            cef_request_get_url_orig = addr.Call<cef_request_get_url_t>();
        } else {
            WLOG_ERROR(L"[AdsBlock] Failed to resolve get_url at index: {}", CMgr::get_url_index);
            if (MemoryW().Unhook(&cef_urlrequest_create_orig, cef_urlrequest_create_hook)) {
                WLOG_INFO(L"[AdsBlock] Unhooked cef_urlrequest_create.");
            }
            return cef_urlrequest_create_orig(request, client, request_context);
        }
    }
    auto u = cef_request_get_url_orig(request);
#endif
    std::wstring url(reinterpret_cast<wchar_t*>(u->str), u->length);
    free_cef_string(u);

    for (const auto& p : CMgr::blocked_urls) {
        if (url.find(p) != std::wstring::npos) {
            WLOG_WARN(L"[AdsBlock] Blocking URL: {}", url.c_str());
            return nullptr;
        }
    }
    WLOG_INFO(L"[AdsBlock] Allowing URL: {}", url.c_str());
    return cef_urlrequest_create_orig(request, client, request_context);
}

cef_zip_reader_t* cef_zip_reader_create_hook(_cef_stream_reader_t* stream) {
    auto reader = cef_zip_reader_create_orig(stream);
    if (!reader) return nullptr;

    MemoryW cef(L"libcef.dll");
    if (auto addr = cef.AtAddress(reader).Resolve(CMgr::get_file_name_index * sizeof(void*))) {
        cef_zip_reader_get_file_name_orig = addr.Call<cef_zip_reader_get_file_name_t>();
        WLOG_INFO(L"[BannerBlock] Resolved get_file_name.");
    } else {
        WLOG_ERROR(L"[BannerBlock] Failed to resolve get_file_name at index {}.", CMgr::get_file_name_index);
    }
    
    if (auto addr = cef.AtAddress(reader).Resolve(CMgr::get_file_size_index * sizeof(void*))) {
        cef_zip_reader_get_file_size_orig = addr.Call<cef_zip_reader_get_file_size_t>();
        if (addr.Hook(&cef_zip_reader_get_file_size_orig, cef_zip_reader_get_file_size_hook)) {
            WLOG_INFO(L"[BannerBlock] Hooked get_file_size.");
        } else {
            WLOG_ERROR(L"[BannerBlock] Failed to hook get_file_size.");
        }
    } else {
        WLOG_ERROR(L"[BannerBlock] Failed to resolve get_file_size at index {}.", CMgr::get_file_size_index);
    }

    if (auto addr = cef.AtAddress(reader).Resolve(CMgr::open_file_index * sizeof(void*))) {
        cef_zip_reader_open_file_orig = addr.Call<cef_zip_reader_open_file_t>();
        WLOG_INFO(L"[BannerBlock] Resolved open_file.");
    } else {
        WLOG_ERROR(L"[BannerBlock] Failed to resolve open_file at index {}.", CMgr::open_file_index);
    }

    if (auto addr = cef.AtAddress(reader).Resolve(CMgr::close_file_index * sizeof(void*))) {
        cef_zip_reader_close_file_orig = addr.Call<cef_zip_reader_close_file_t>();
        WLOG_INFO(L"[BannerBlock] Resolved close_file.");
    } else {
        WLOG_ERROR(L"[BannerBlock] Failed to resolve close_file at index {}.", CMgr::close_file_index);
    }

    if (auto addr = cef.AtAddress(reader).Resolve(CMgr::read_file_index * sizeof(void*))) {
        cef_zip_reader_read_file_orig = addr.Call<cef_zip_reader_read_file_t>();
        if (addr.Hook(&cef_zip_reader_read_file_orig, cef_zip_reader_read_file_hook)) {
            WLOG_INFO(L"[BannerBlock] Hooked read_file.");
        } else {
            WLOG_ERROR(L"[BannerBlock] Failed to hook read_file.");
        }
    } else {
        WLOG_ERROR(L"[BannerBlock] Failed to resolve read_file at index {}.", CMgr::read_file_index);
    }

    if (MemoryW().Unhook(&cef_zip_reader_create_orig, cef_zip_reader_create_hook)) {
        WLOG_INFO(L"[BannerBlock] Unhooked cef_zip_reader_create.");
    }

    return reader;
}

bool apply_patches(std::string& s, CMgr::PatchInfoMap& patches, const std::wstring& file) {
    bool any = false;

    for (auto& [key, p] : patches) {
        std::regex re(EncodingUtils::WStringToUTF8(p.pattern));
        std::string val = EncodingUtils::WStringToUTF8(p.value), rep;
        std::smatch m; std::vector<intptr_t> fr;
        auto do_patch = [&](size_t pos) {
            if (pos >= s.size() || !std::regex_search(s.cbegin() + pos, s.cend(), m, re) || m.position() != 0) return;
            rep.clear();
            for (size_t i = 0; i < val.size(); ) {
                if (val[i] == '\\' && i + 1 < val.size() && (val[i+1] == '$' || val[i+1] == '\\')) {
                    rep += val[++i];
                    ++i;
                } else if (val[i] == '$') {
                    size_t j = i + 1;
                    bool br = (j < val.size() && val[j] == '{');
                    if (br) ++j;
                    size_t st = j;
                    while (j < val.size() && isdigit(val[j])) ++j;
                    if (j > st && (!br || (j < val.size() && val[j] == '}'))) {
                        int g = std::stoi(val.substr(st, j - st));
                        if (g < (int)m.size()) {
                            rep += m.str(g);
                            i = br ? j + 1 : j;
                            continue;
                        }
                    }
                    rep += '$'; ++i;
                } else { rep += val[i++]; }
            }
            s.replace(pos, m.length(), rep);
            fr.push_back(pos);
            any = true;
            WLOG_INFO(L"[BannerBlock] '{}' patched at {} in {}.", key.c_str(), pos, file.c_str());
            // WLOG_DEBUG(L"{:#X} => {} | {}", pos, key.c_str(), EncodingUtils::UTF8ToWString(s.substr(pos, 100)));
        };

        for (auto pos : p.rvas) do_patch(pos);
        if (fr.empty()) {
            if (p.replace_all) {
                for (auto it = std::sregex_iterator(s.begin(), s.end(), re), end = std::sregex_iterator(); it != end; ++it) {
                    do_patch(it->position());
                }
            } else if (std::regex_search(s, m, re)) {
                do_patch(m.position());
            }
            if (fr.empty()) { WLOG_ERROR(L"[BannerBlock] '{}' not found in {}.", key.c_str(), file.c_str()); }
        }

        p.rvas = std::move(fr);
        if (any) CMgr::FlushToJson();
    }
    return any;
}

#ifdef USE_PATCHED_FILE_CACHE
static std::unordered_map<std::wstring, std::vector<char>> g_patched_files;
#endif

int __stdcall cef_zip_reader_read_file_hook(_cef_zip_reader_t* self, void* buffer, size_t bufferSize) {
    auto n = get_file_name(self);
#ifdef USE_PATCHED_FILE_CACHE
    if (const auto& it = g_patched_files.find(n); it != g_patched_files.end()) {
        auto w = std::min(it->second.size(), bufferSize);
        memcpy(buffer, it->second.data(), w);
        return static_cast<int>(w);
    }
    return read_file(self, buffer, bufferSize);
#else
    auto r = read_file(self, buffer, bufferSize); if (r <= 0) return r;
    if (const auto& it = CMgr::file_patches.find(n); it != CMgr::file_patches.end()) {
        std::string c(reinterpret_cast<char*>(buffer), r);
        if (apply_patches(c, it->second, n)) {
            auto w = std::min(c.size(), bufferSize);
            memcpy(buffer, c.data(), w);
            return static_cast<int>(w);
        }
    }
    return r;
#endif
}

int64_t __stdcall cef_zip_reader_get_file_size_hook(_cef_zip_reader_t* self) {
    auto s = cef_zip_reader_get_file_size_orig(self);
    auto n = get_file_name(self);
#ifdef USE_PATCHED_FILE_CACHE
    if (auto it = CMgr::file_patches.find(n); it != CMgr::file_patches.end()) {
        std::vector<char> buffer(s);
        if (open_file(self, nullptr)) {
            auto rb = read_file(self, buffer.data(), static_cast<size_t>(s));
            close_file(self);
            if (rb <= 0) { return s; }

            std::string c(buffer.data(), rb);
            if (apply_patches(c, it->second, n)) {
                g_patched_files[n] = std::vector<char>(c.begin(), c.end());
                return static_cast<int64_t>(c.size());
            }  
        }
    }
    return s;
#else
    return CMgr::file_patches.contains(n) ? s * 1.25 : s;
#endif
}

static void free_cef_string(cef_string_userfree_utf16_t s) {
    static auto orig = []() -> cef_string_userfree_utf16_free_t {
        if (auto addr = MemoryW(L"libcef.dll").GetFunction("cef_string_userfree_utf16_free"))
            return addr.Call<cef_string_userfree_utf16_free_t>();
        WLOG_ERROR(L"[Function] free_cef_string: orig missing");
        return nullptr;
    }();
    if (s && orig) orig(s);
}

static std::wstring get_file_name(_cef_zip_reader_t* self) {
    if (!cef_zip_reader_get_file_name_orig) {
        WLOG_ERROR(L"[Function] get_file_name: orig missing");
        return {};
    }
    if (auto s = cef_zip_reader_get_file_name_orig(self)) {
        std::wstring name(reinterpret_cast<wchar_t*>(s->str), s->length);
        free_cef_string(s);
        return name;
    }
    return {};
}

static bool open_file(_cef_zip_reader_t* self, const cef_string_utf16_t* password) {
    if (!cef_zip_reader_open_file_orig) {
        WLOG_ERROR(L"[Function] open_file: orig missing");
        return false;
    }
    return cef_zip_reader_open_file_orig(self, password) > 0;
}

static void close_file(_cef_zip_reader_t* self) {
    if (!cef_zip_reader_close_file_orig) {
        WLOG_ERROR(L"[Function] close_file: orig missing");
        return;
    }
    cef_zip_reader_close_file_orig(self);
}

static int read_file(_cef_zip_reader_t* self, void* buffer, size_t bufferSize) {
    if (!cef_zip_reader_read_file_orig) {
        WLOG_ERROR(L"[Function] read_file: orig missing");
        return -1;
    }
    return cef_zip_reader_read_file_orig(self, buffer, bufferSize);
}