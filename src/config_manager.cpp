#include "config_manager.h"
#include "modify.h"
#include <fstream>
#include <sstream>
#include "BasicUtils/logger.hpp"
#include "BasicUtils/utils.hpp"

using namespace utils;

#ifndef NDEBUG
#include <cstddef>
#define VALIDATE_INDEX(var, Type, member) do { \
    constexpr size_t expected = offsetof(Type, member) / sizeof(void*); \
    if ((var) != expected) { \
        (var) = expected; \
        WLOG_WARN(L"[ConfigManager] '{}' updated to correct index: {}", L#member, var); \
    } \
} while (0)
#endif

void ConfigManager::Initialize()
{
    SyncIni();
    WLOG_INIT(L"blockthespot.log", IsFeatureEnabled(L"enable_logging"));

    if (FileUtilsW::GetFileVersion(L"Spotify.exe", m_version)) {
        WLOG_INFO(L"[ConfigManager] Spotify version: {}", m_version);
    } else {
        WLOG_WARN(L"[ConfigManager] Could not retrieve Spotify version.");
    }

    if (!Load()) return;

    UpdateCaches();

    if (IsFeatureEnabled(L"enable_dev_mode") && !dev_mode_patch.pattern.empty())
        CreateThread(NULL, 0, DevMode, NULL, 0, NULL);

    if (IsFeatureEnabled(L"enable_ads_block") && (get_url_index != -1 && !blocked_urls.empty()))
        CreateThread(NULL, 0, AdsBlock, NULL, 0, NULL);

    if (IsFeatureEnabled(L"enable_banner_block") && (read_file_index != -1 && get_file_name_index != -1 && !file_patches.empty()))
        CreateThread(NULL, 0, BannerBlock, NULL, 0, NULL);

    if (IsFeatureEnabled(L"enable_auto_update"))
        CreateThread(NULL, 0, AutoUpdate, NULL, 0, NULL);
}

bool ConfigManager::Load()
{
    std::wstring content;
    if (!FileUtilsW::ReadFile(m_path, content)) {
        m_settings = DefaultSettings();
        return Save();
    }
    try { m_settings = wjson::parse(content); return true; }
    catch (...) { m_settings = DefaultSettings(); return Save(); }
}

bool ConfigManager::Save()
{
    std::scoped_lock lock(m_mutex);
    return FileUtilsW::WriteFile(m_path, m_settings.dump(2));
}

wjson ConfigManager::DefaultSettings()
{
    return {
#ifndef NDEBUG
        { L"BuildDate", std::format(L"{:%Y-%m-%d}", std::chrono::year_month_day{ std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now()) }) },
        //{L"Version", m_version},
#else
        {L"BuildDate", L"2025-10-09"}, // for dll
        //{L"Version", L"1.2.73.474"}, // for json
#endif
        {L"Indices", {
            {L"cef_request_get_url", 6},
            {L"cef_zip_reader_get_file_name", 9},
            {L"cef_zip_reader_get_file_size", 10},
            {L"cef_zip_reader_open_file", 12},
            {L"cef_zip_reader_close_file", 13},
            {L"cef_zip_reader_read_file", 14}
        }},
        {L"DevMode", {
            {L"x64", PatchInfo(L"D1 EB ?? ?? ?? 48 8B 95 ?? ?? ?? ?? 48 83 FA 0F", L"B3 01 90", 2)},
            {L"x32", PatchInfo(L"25 01 FF FF FF 89 ?? ?? ?? FF FF", L"B8 03 00")}
        }},
        {L"BlockedUrls", {
            L"/ads/",
            L"/ad-logic/",
            L"/gabo-receiver-service/"
        }},
        {L"FilePatch", {
            {L"6256.css", {
                {L"hide_home_recs", PatchInfo(L"(\\.Y89c1_2SAoZFkICK7WVp\\{)", L"$1display:none !important;")}
            }},
            {L"dwp-home-chips-row.css", {
                {L"hide_home_chips_row", PatchInfo(L"(\\.c8Z2jJUocJTdV9g741cp\\b[^\\{]*\\{)", L"$1display:none !important;")}
            }},
            {L"home-hpto.css", {
                {L"hide_hpto_banner", PatchInfo(L"(\\.Mvhjv8IKLGjQx94MVOgP\\{display:-webkit-box;display:-ms-flexbox;display:)flex(;)", L"$1none$2")}
            }},
            {L"xpui-root-dialogs.js", {
                {L"disable_in_app_verification", PatchInfo(L"(function[^{]*?\\{)[^}]*?\\.withPath\\(\\\"/verifications/\\\"\\)[^}]*?\\}",
                    L"$1return;}")} // function n(e){return e.build().withHost(s.D8).withPath("/verifications/").withEndpointIdentifier("/verifications/").send()}
            }},
            // {L"xpui-pip-mini-player.js", {
            //     {L"disable_pip_upsell", PatchInfo(L"(return\\(0,D\\.jsx\\)\\(\"div\",\\{[^]*?\"web-player\\.pip-mini-player\\.upsell\\.title\"[^]*?\\)\\))", 
            //         L"return null")} // return(0,D.jsx)("div",{className:Oe,children:(0,D.jsxs)
            // }},
            {L"xpui-snapshot.js", {
                {L"ads_enabled", PatchInfo(L"(adsEnabled:!)0", L"${1}1")},
                {L"ads_hpto_hidden", PatchInfo(L"(isHptoHidden:!)0", L"${1}1")},
                {L"sponsorships", PatchInfo(L"(\\.set\\(\")allSponsorships(\",t\\.sponsorships\\)\\}\\}\\(e,t\\);)", L"$1$2")},
                {L"skip_sentry", PatchInfo(L"sentry\\.io", L"localhost")},
                {L"disable_sentry_init", PatchInfo(L"(function\\s*\\w*\\s*\\([^)]*\\)\\s*\\{)([^}]*?\\.ingest\\.sentry\\.io.*?\\})", L"$1return;$2")}, // function Zk(){if(xE()){;
                {L"disable_browser_metrics", PatchInfo(L"(function\\s*\\w*\\s*\\([^)]*\\)\\s*\\{)([^}]*?\\w+\\.BrowserMetrics\\.getPageLoadTime\\(\\).*?\\})", L"$1return;$2")}, // function Gk(e){Ub.BrowserMetrics
                {L"hpto_enabled", PatchInfo(L"(hptoEnabled:!)(0)", L"${1}1")},
                {L"enable_premium_features", PatchInfo(L"(await\\s+\\w+\\(\\s*([\\w.]+)\\.initialUser\\s*,\\s*\\2\\.initialProductState\\s*,.*?\\);)",
                    L"$2.initialProductState={...$2.initialProductState,product:'premium',catalogue:'premium',ads:'0'},$1")} // await Kk(l.initialUser, l.initialProductState, z);
            }}
    }}
    };
}

void ConfigManager::SyncIni()
{
    const wchar_t* path = L".\\config.ini";

    struct Entry { const wchar_t* section; const wchar_t* key; bool def; };
    static constexpr Entry entries[] = {
        {L"Config", L"enable_ads_block", true},
        {L"Config", L"enable_banner_block", true},
        {L"Config", L"enable_dev_mode", true},
        {L"Config", L"enable_auto_update", true},
        {L"Config", L"enable_logging", false},
        
        {L"FilePatch", L"hide_home_recs", false},
        {L"FilePatch", L"hide_home_chips_row", false}
    };

    for (const auto& e : entries) {
        std::wstring val = FileUtilsW::ReadIni(path, e.section, e.key);
        if (val.empty())
            FileUtilsW::WriteIni(path, e.section, e.key, e.def ? L"1" : L"0");

        bool enabled = val.empty() ? e.def : (!_wcsicmp(val.c_str(), L"1"));
        m_config[e.section][e.key] = enabled;
    }
}

bool ConfigManager::IsFeatureEnabled(const std::wstring& key, const std::wstring& section, bool fallback)
{
    auto sit = m_config.find(section);
    if (sit == m_config.end()) return fallback;
    auto kit = sit->second.find(key);
    if (kit == sit->second.end()) return fallback;
    return kit->second;
}

size_t ConfigManager::GetIndex(const std::wstring& key)
{
    try {
        return m_settings.at(L"Indices").at(key).get<size_t>();
    } catch (...) {
        WLOG_WARN(L"[ConfigManager] Missing index '{}'", key.c_str());
        return -1;
    }
}

std::vector<std::wstring> ConfigManager::GetBlockedUrls()
{
    std::vector<std::wstring> result;
    try {
        for (const auto& item : m_settings.at(L"BlockedUrls")) {
            result.push_back(item.get<std::wstring>());
        }
    } catch (...) {
        WLOG_WARN(L"[ConfigManager] No blocked URLs found in settings.");
    }
    return result;
}

ConfigManager::FilePatchMap ConfigManager::GetFilePatches()
{
    FilePatchMap result;
    try {
        const auto& filePatch = m_settings.at(L"FilePatch");
        for (const auto& file_entry : filePatch) {
            const auto& file = file_entry.key;
            const auto& patch_items = file_entry.value;

            for (const auto& patch_entry : patch_items) {
                const auto& key = patch_entry.key;
                const auto& patch_data = patch_entry.value;
                result[file][key] = patch_data.get<PatchInfo>();
            }
        }
    } catch (...) {
        WLOG_ERROR(L"[ConfigManager] FilePatch configuration could not be loaded.");
    }
    return result;
}

PatchInfo ConfigManager::GetDevModePatch()
{
    PatchInfo result;
    try {
        m_settings.at(L"DevMode").at((sizeof(void*) == 8 ? L"x64" : L"x32")).get_to<PatchInfo>(result);
    } catch (...) {
        WLOG_ERROR(L"[ConfigManager] DevMode patch configuration could not be loaded.");
    }
    return result;
}

bool ConfigManager::FlushToJson()
{
    try
    {
        wjson s = m_settings;
        s[L"Indices"] = {
            {L"cef_request_get_url", get_url_index},
            {L"cef_zip_reader_read_file", read_file_index},
            {L"cef_zip_reader_get_file_name", get_file_name_index},
            {L"cef_zip_reader_get_file_size", get_file_size_index},
            {L"cef_zip_reader_open_file", open_file_index},
            {L"cef_zip_reader_close_file", close_file_index}
        };
        s[L"BlockedUrls"] = blocked_urls;

        const auto arch = sizeof(void*)==8 ? L"x64" : L"x32";
        auto dev = s[L"DevMode"];
        dev[arch] = { {L"pattern", dev_mode_patch.pattern}, {L"value", dev_mode_patch.value} };
        if (dev_mode_patch.offset) dev[arch][L"offset"] = dev_mode_patch.offset;
        if (!dev_mode_patch.rvas.empty()) dev[arch][L"rvas"] = dev_mode_patch.rvas;
        s[L"DevMode"] = std::move(dev);

        wjson fp;
        for (auto& [f, ps] : file_patches)
            for (auto& [k, p] : ps)
                fp[f][k] = p;
        s[L"FilePatch"] = std::move(fp);

        return (s != m_settings) ? (m_settings = std::move(s), Save()) : true;
    } catch (...) {
        WLOG_ERROR(L"[ConfigManager] FlushToJson failed");
    }
    return false;
}

void ConfigManager::UpdateCaches()
{
    blocked_urls = GetBlockedUrls();
    file_patches = GetFilePatches();
    dev_mode_patch = GetDevModePatch();

    get_url_index = GetIndex(L"cef_request_get_url");
    get_file_name_index = GetIndex(L"cef_zip_reader_get_file_name");
    get_file_size_index = GetIndex(L"cef_zip_reader_get_file_size");
    open_file_index = GetIndex(L"cef_zip_reader_open_file");
    close_file_index = GetIndex(L"cef_zip_reader_close_file");
    read_file_index = GetIndex(L"cef_zip_reader_read_file");

#ifndef NDEBUG
    VALIDATE_INDEX(get_url_index, cef_request_t, get_url);
    VALIDATE_INDEX(get_file_name_index, cef_zip_reader_t, get_file_name);
    VALIDATE_INDEX(get_file_size_index, cef_zip_reader_t, get_file_size);
    VALIDATE_INDEX(open_file_index, cef_zip_reader_t, open_file);
    VALIDATE_INDEX(close_file_index, cef_zip_reader_t, close_file);
    VALIDATE_INDEX(read_file_index, cef_zip_reader_t, read_file);
#endif
}

bool ConfigManager::SyncRemote(const wjson& remote)
{
    try {
        const std::vector<std::wstring> ign = { L"BuildDate", L"rvas" };
        auto is_ig = [&](const std::wstring& k) { return std::find(ign.begin(), ign.end(), k) != ign.end(); };
        
        auto strip = [&](auto&& self, const wjson& v) -> wjson {
            if (v.is_object()) {
                wjson out = wjson::object_t{};
                for (auto pr : v) {
                    if (is_ig(pr.key)) continue;
                    const auto& val = pr.value;
                    if (val.is_object() || val.is_array())
                        out[pr.key] = self(self, val);
                    else
                        out[pr.key] = val;
                }
                return out;
            }
            if (v.is_array()) {
                wjson out = wjson::array_t{};
                for (auto it : v) {
                    if (it.value.is_object() || it.value.is_array())
                        out.push_back(self(self, it.value));
                    else
                        out.push_back(it.value);
                }
                return out;
            }
            return v;
        };

        if (strip(strip, remote) == strip(strip, m_settings)) {
            WLOG_INFO(L"[SyncRemote] no effective changes.");
            return false;
        }

        auto merge = [&](auto&& self, const wjson& s, wjson& d) -> bool {
            bool chg = false;

            if (!s.is_object()) {
                if (!(d == s)) { d = s; return true; }
                return false;
            }

            for (auto p : s) {
                if (is_ig(p.key)) continue;

                if (p.value.is_object()) {
                    if (!d.contains(p.key) || !d[p.key].is_object()) {
                        if (!d.contains(p.key) || d[p.key] != p.value) {
                            d[p.key] = p.value;
                            chg = true;
                        }
                    }
                    else {
                        if (self(self, p.value, d[p.key])) chg = true;
                    }
                }
                else {
                    if (!d.contains(p.key) || d[p.key] != p.value) {
                        d[p.key] = p.value;
                        chg = true;
                    }
                }
            }

            return chg;
        };

        if (!merge(merge, remote, m_settings)) {
            WLOG_INFO(L"[SyncRemote] merge done but no changes applied.");
            return false;
        }

        if (!Save()) {
            WLOG_ERROR(L"[SyncRemote] save failed.");
            return false;
        }

        UpdateCaches();

        WLOG_INFO(L"[SyncRemote] settings updated.");
        return true;
    }
    catch (...) {
        WLOG_ERROR(L"[SyncRemote] exception during sync.");
        return false;
    }
}

DWORD WINAPI ConfigManager::AutoUpdate(LPVOID)
{
    auto u = std::format(L"https://raw.githubusercontent.com/mrpond/BlockTheSpot/master/{}", m_path);
    auto ask_update = []() -> bool {
        static int c = -1;
        if (c == -1)
            c = MessageBoxW(nullptr, L"New version available. Update?", L"BlockTheSpot", MB_YESNO | MB_ICONQUESTION);
        return c == IDYES;
    };

    while (IsFeatureEnabled(L"enable_auto_update")) {
        if (std::wstring p; HttpUtils::Get(u, p)) {
            try {
                auto r = wjson::parse(p);
                if (r.at(L"BuildDate").get_string() != m_settings.at(L"BuildDate").get_string() && ask_update()) { // dll update
                    _wsystem(
                        L"powershell -Command \"& {"
                        L"[Net.ServicePointManager]::SecurityProtocol = "
                        L"[Net.SecurityProtocolType]::Tls12; "
                        L"Invoke-WebRequest -UseBasicParsing "
                        L"'https://raw.githubusercontent.com/mrpond/BlockTheSpot/master/install.ps1' "
                        L"| Invoke-Expression}\""
                    );
                }
                
                SyncRemote(r);
            } catch (...) { 
                WLOG_ERROR(L"[AutoUpdate] Unknown exception during update check.");
            }
        } else {
            WLOG_WARN(L"[AutoUpdate] HTTP GET failed for {}", u);
        }
        std::this_thread::sleep_for(std::chrono::hours(1));
    }
    return 0;
}