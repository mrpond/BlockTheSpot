#pragma once
#include <windows.h>
#include <vector>
#include <string>
#include <cstdint>
#include <map>
#include <mutex>
#include "BasicUtils/json.hpp"

struct PatchInfo {
    std::wstring pattern;
    std::wstring value;
    size_t offset;
    bool replace_all;
    std::vector<intptr_t> rvas;

    PatchInfo() noexcept : pattern(), value(), offset(0), replace_all(false), rvas() {}
    PatchInfo(std::wstring pat, std::wstring val, size_t off = 0, bool all = false)
        : pattern(std::move(pat)), value(std::move(val)), offset(off), replace_all(all), rvas() {}
};

// JSON_DEFINE_TYPE(PatchInfo, pattern, value, offset, replace_all, rvas);
namespace jsn {
    inline void to_json(wjson& j, PatchInfo const& p) {
        j = wjson{{L"pattern", p.pattern}, {L"value", p.value}};
        if (p.offset) j[L"offset"] = p.offset;
        if (p.replace_all) j[L"replace_all"] = p.replace_all;
        if (!p.rvas.empty()) j[L"rvas"] = p.rvas;
    }

    inline void from_json(wjson const& j, PatchInfo& p) {
        p.pattern= j.at(L"pattern").get<std::wstring>();
        p.value= j.at(L"value").get<std::wstring>();
        if (j.contains(L"offset")) p.offset = j.at(L"offset").get<size_t>();
        if (j.contains(L"replace_all")) p.replace_all = j.at(L"replace_all").get<bool>();
        if (j.contains(L"rvas")) p.rvas = j.at(L"rvas").get<std::vector<intptr_t>>();
    }
}

class ConfigManager {
public:
    using PatchInfoMap = std::map<std::wstring, PatchInfo>;
    using FilePatchMap = std::map<std::wstring, PatchInfoMap>;
    static void Initialize();
    static bool FlushToJson();
    static bool IsFeatureEnabled(const std::wstring& key, const std::wstring& section = L"Config", bool fallback = false);

    inline static size_t get_url_index;
    inline static size_t get_file_name_index;
    inline static size_t get_file_size_index;
    inline static size_t open_file_index;
    inline static size_t close_file_index;
    inline static size_t read_file_index;
    inline static std::vector<std::wstring> blocked_urls;
    inline static FilePatchMap file_patches;
    inline static PatchInfo dev_mode_patch;

private:
    static void SyncIni();
    static bool Load();
    static bool Save();
    static wjson DefaultSettings();

    static size_t GetIndex(const std::wstring& key);
    static std::vector<std::wstring> GetBlockedUrls();
    static FilePatchMap GetFilePatches();
    static PatchInfo GetDevModePatch();

    static void UpdateCaches();

    static bool SyncRemote(const wjson& remote);
    static DWORD WINAPI AutoUpdate(LPVOID lpParam);

    inline static std::map<std::wstring, std::map<std::wstring, bool>> m_config;
    inline static wjson m_settings;
    inline static std::mutex m_mutex;
    inline static const std::wstring m_path = L"blockthespot_settings.json";
    inline static std::wstring m_version;
};
