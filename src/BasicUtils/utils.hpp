#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <filesystem>
#include <cctype>
#include <type_traits>
#include <iomanip>
#include <cwctype>

#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "version.lib")

namespace utils
{
    template <typename CharT>
    struct StringUtils {
        using string_t = std::basic_string<CharT>;
        using view_t = std::basic_string_view<CharT>;

        static inline CharT ToLower(CharT c) noexcept {
            if constexpr (std::is_same_v<CharT, char>) return static_cast<CharT>(std::tolower(c));
            else return static_cast<CharT>(std::towlower(c));
        }
        static inline CharT ToUpper(CharT c) noexcept {
            if constexpr (std::is_same_v<CharT, char>) return static_cast<CharT>(std::toupper(c));
            else return static_cast<CharT>(std::towupper(c));
        }

        static inline bool Contains(view_t s, view_t sub, bool case_sensitive = true) {
            if (case_sensitive) return s.find(sub) != view_t::npos;
            string_t ls(s);
            string_t lsub(sub);
            std::transform(ls.begin(), ls.end(), ls.begin(), ToLower);
            std::transform(lsub.begin(), lsub.end(), lsub.begin(), ToLower);
            return ls.find(lsub) != string_t::npos;
        }

        static inline bool Equals(view_t a, view_t b, bool case_sensitive = true) {
            if (case_sensitive) return a == b;
            if (a.size() != b.size()) return false;
            for (size_t i = 0; i < a.size(); i++) {
                if (ToLower(a[i]) != ToLower(b[i])) return false;
            }
            return true;
        }

        template <typename T, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, view_t>>>
        static inline string_t ToBasicString(const T& v) {
            std::basic_ostringstream<CharT> oss; oss << v;
            return oss.str();
        }
        static inline string_t ToBasicString(view_t sv) {
            return string_t(sv);
        }
    };

    using StringUtilsA = StringUtils<char>;
    using StringUtilsW = StringUtils<wchar_t>;

    template <typename CharT>
    struct HexUtils {
        using string_t = std::basic_string<CharT>;
        using view_t = std::basic_string_view<CharT>;

        static inline int HexDigit(CharT c) {
            if (c >= (CharT)'0' && c <= (CharT)'9') return c - (CharT)'0';
            if (c >= (CharT)'A' && c <= (CharT)'F') return c - (CharT)'A' + 10;
            if (c >= (CharT)'a' && c <= (CharT)'f') return c - (CharT)'a' + 10;
            throw std::invalid_argument("Invalid hex character");
        }

        template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
        static inline string_t ToHex(T v) {
            std::basic_ostringstream<CharT> oss;
            oss.imbue(std::locale::classic());
            oss << std::uppercase << std::hex << static_cast<std::make_unsigned_t<T>>(v);
            return oss.str();
        }

        template <typename Container, typename V = typename Container::value_type, typename = std::enable_if_t<std::is_integral_v<V>>>
        static inline string_t ToHex(const Container& c, bool insert_spaces = false) {
            std::basic_ostringstream<CharT> oss;
            oss.imbue(std::locale::classic());
            oss << std::uppercase << std::hex << std::setfill((CharT)'0');
            for (size_t i = 0; i < c.size(); i++) {
                if constexpr (std::is_same_v<CharT, char>) {
                    oss << std::setw(2) << static_cast<int>(c[i]);
                } else {
                    auto hex = ToHex<CharT>(static_cast<V>(c[i]));
                    if (hex.size() < 2) oss << (CharT)'0';
                    oss << hex;
                }
                if (insert_spaces && i + 1 < c.size()) oss << (CharT)' ';
            }
            return oss.str();
        }

        template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
        static inline T FromHex(view_t hex_str, bool ignore_invalid = true) {
            T value = 0;
            int high = -1;
            for (CharT c : hex_str) {
                int digit;
                try { digit = HexDigit(c); }
                catch (...) { if (ignore_invalid) continue; else throw; }
                if (high < 0) high = digit;
                else {
                    value = static_cast<T>((value << 8) | ((high << 4) | digit));
                    high = -1;
                }
            }
            if (high >= 0) value = static_cast<T>((value << 4) | high);
            return value;
        }

        template <typename Container, typename V = typename Container::value_type, typename = std::enable_if_t<std::is_integral_v<V>>>
        static inline Container FromHexToBytes(StringUtils<CharT>::view_t hex_str, bool ignore_invalid = true) {
            Container result;
            result.reserve(hex_str.size()/2);
            int high = -1;
            for (CharT c : hex_str) {
                int digit;
                try { digit = HexDigit(c); }
                catch (...) { if (ignore_invalid) continue; else throw; }
                if (high < 0) high = digit;
                else { result.push_back(static_cast<V>((high<<4)|digit)); high = -1; }
            }
            if (high >= 0) result.push_back(static_cast<V>(high));
            return result;
        }
    };

    using HexUtilsA = HexUtils<char>;
    using HexUtilsW = HexUtils<wchar_t>;

    struct EncodingUtils {
        static inline std::wstring UTF8ToWString(const std::string& utf8) {
            if (utf8.empty()) return {};
            int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), (int)utf8.size(), nullptr, 0);
            std::wstring wstr(size_needed, 0);
            MultiByteToWideChar(CP_UTF8, 0, utf8.data(), (int)utf8.size(), &wstr[0], size_needed);
            return wstr;
        }

        static inline std::string WStringToUTF8(const std::wstring& wstr) {
            if (wstr.empty()) return {};
            int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
            std::string utf8(size_needed, 0);
            WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), &utf8[0], size_needed, nullptr, nullptr);
            return utf8;
        }
    };

    template <typename CharT>
    struct FileUtils {
        using string_t = std::basic_string<CharT>;
        using view_t = std::basic_string_view<CharT>;

        static inline bool ReadFile(const string_t& path, string_t& out) noexcept {
            out.clear();
            if (!std::filesystem::exists(path)) return false;
            std::basic_ifstream<CharT> ifs(path, std::ios::binary);
            if (!ifs) return false;
            std::basic_ostringstream<CharT> oss;
            oss << ifs.rdbuf();
            out = oss.str();
            return true;
        }

        static inline bool WriteFile(const string_t& path, view_t data) noexcept {
            std::basic_ofstream<CharT> ofs(path, std::ios::binary);
            if (!ofs) return false;
            ofs.write(data.data(), data.size());
            return true;
        }

        static inline string_t ReadIni(const string_t& path, const string_t& section, const string_t& key, const string_t& value = {}) {
            constexpr DWORD size = 1024;
            DWORD cr; std::vector<CharT> buffer(size);
            if constexpr (std::is_same_v<CharT, char>) {
                cr = GetPrivateProfileStringA(section.c_str(), key.c_str(), value.c_str(), buffer.data(), size, path.c_str());
            } else {
                cr = GetPrivateProfileStringW(section.c_str(), key.c_str(), value.c_str(), buffer.data(), size, path.c_str());
            }
            return string_t(buffer.data(), cr);
        }

        static inline bool WriteIni(const string_t& path, const string_t& section, const string_t& key, const string_t& value) {
            if constexpr (std::is_same_v<CharT, char>) {
                return WritePrivateProfileStringA(section.c_str(), key.c_str(), value.c_str(), path.c_str()) != FALSE;
            } else {
                return WritePrivateProfileStringW(section.c_str(), key.c_str(), value.c_str(), path.c_str()) != FALSE;
            }
        }

        static inline bool FileExists(const std::filesystem::path& p) noexcept { return std::filesystem::exists(p); }
        static inline bool DeleteFile(const std::filesystem::path& p) noexcept { return std::filesystem::remove(p); }
        static inline std::vector<std::filesystem::path> ListFiles(const std::filesystem::path& d) {
            std::vector<std::filesystem::path> files;
            if (!std::filesystem::is_directory(d)) return files;
            for (auto& e : std::filesystem::directory_iterator(d)) files.push_back(e.path());
            return files;
        }

        static inline bool GetFileVersion(const string_t& p, string_t& out) noexcept {
            out.clear(); if (p.empty()) return false;
            DWORD h = 0, sz = 0; std::vector<std::byte> buf;
            if constexpr (std::is_same_v<CharT, char>) {
                if (!(sz = GetFileVersionInfoSizeA(p.c_str(), &h))) return false;
                buf.resize(sz); VS_FIXEDFILEINFO* f = nullptr;
                if (!GetFileVersionInfoA(p.c_str(), 0, sz, reinterpret_cast<LPVOID>(buf.data()))
                    || !VerQueryValueA(buf.data(), "\\", reinterpret_cast<LPVOID*>(&f), nullptr)) return false;
                auto ms = f->dwFileVersionMS, ls = f->dwFileVersionLS;
                out = std::format("{}.{}.{}.{}", HIWORD(ms), LOWORD(ms), HIWORD(ls), LOWORD(ls));
            }
            else {
                if (!(sz = GetFileVersionInfoSizeW(p.c_str(), &h))) return false;
                buf.resize(sz); VS_FIXEDFILEINFO* f = nullptr;
                if (!GetFileVersionInfoW(p.c_str(), 0, sz, reinterpret_cast<LPVOID>(buf.data()))
                    || !VerQueryValueW(buf.data(), L"\\", reinterpret_cast<LPVOID*>(&f), nullptr)) return false;
                auto ms = f->dwFileVersionMS, ls = f->dwFileVersionLS;
                out = std::format(L"{}.{}.{}.{}", HIWORD(ms), LOWORD(ms), HIWORD(ls), LOWORD(ls));
            }
            return true;
        }
    };

    using FileUtilsA = FileUtils<char>;
    using FileUtilsW = FileUtils<wchar_t>;

    struct HttpUtils {
        static bool Get(std::wstring_view url, std::wstring& response) {
            URL_COMPONENTS comp{};
            comp.dwStructSize = sizeof(comp);
            wchar_t host[256] = {}, path[4096] = {};
            comp.lpszHostName = host; comp.dwHostNameLength = std::size(host);
            comp.lpszUrlPath = path; comp.dwUrlPathLength = std::size(path);

            if (!WinHttpCrackUrl(url.data(), static_cast<DWORD>(url.size()), 0, &comp))
                return false;

            auto closer = [](HINTERNET h) { if (h) WinHttpCloseHandle(h); };
            using win_handle = std::unique_ptr<std::remove_pointer_t<HINTERNET>, decltype(closer)>;

            win_handle session(WinHttpOpen(
                L"Mozilla/5.0 (Windows NT 10.0; Win64; x64)",
                WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                WINHTTP_NO_PROXY_NAME,
                WINHTTP_NO_PROXY_BYPASS,
                0), closer);
            if (!session) return false;

            win_handle connect(WinHttpConnect(session.get(), comp.lpszHostName, comp.nPort, 0), closer);
            if (!connect) return false;

            DWORD flags = ((comp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0) | WINHTTP_FLAG_REFRESH;
            win_handle request(WinHttpOpenRequest(connect.get(), L"GET", comp.lpszUrlPath, nullptr,
                WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags), closer);

            DWORD disableCache = WINHTTP_DISABLE_KEEP_ALIVE;
            WinHttpSetOption(request.get(), WINHTTP_OPTION_DISABLE_FEATURE, &disableCache, sizeof(disableCache));

            if (!request) return false;
            if (!WinHttpSendRequest(request.get(), WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
                !WinHttpReceiveResponse(request.get(), nullptr))
                return false;

            std::string temp;
            DWORD available = 0;
            while (WinHttpQueryDataAvailable(request.get(), &available) && available) {
                std::string buffer(available, '\0');
                DWORD read = 0;
                if (!WinHttpReadData(request.get(), buffer.data(), available, &read) || read == 0)
                    break;
                temp.append(buffer, 0, static_cast<size_t>(read));
            }

            response.assign(temp.begin(), temp.end());
            return true;
        }
    };
} // namespace utils
