#include "pch.h"

#include <chrono>
#include <thread>

void SettingsManager::Init()
{
    SyncConfigFile();
    Logger::Init(L"blockthespot.log", SettingsManager::m_config.at(L"Enable_Log"));

    m_app_settings_file = L"blockthespot_settings.json";
    if (!Load()) {
        if (!Save()) {
            Log(Utils::FormatString(L"Failed to open settings file: {}", m_app_settings_file), LogLevel::Error);
        }
    }

    auto thread = CreateThread(NULL, 0, Update, NULL, 0, NULL);
    if (thread != nullptr) {
        CloseHandle(thread);
    }
}

bool SettingsManager::Save()
{
    m_block_list = { L"/ads/", L"/ad-logic/", L"/gabo-receiver-service/" };

    m_zip_reader = {
        {L"home-hpto.css", {
            {L"hptocss", {
                {L"Signature", L".WiPggcPDzbwGxoxwLWFf{display:-webkit-box;display:-ms-flexbox;display:flex;"},
                {L"Value", L".WiPggcPDzbwGxoxwLWFf{display:-webkit-box;display:-ms-flexbox;display:none;"},
                {L"Offset", 0},
                {L"Fill", 0},
                {L"Address", 0}
            }}
        }},
        {L"xpui.js", {
            {L"adsEnabled", {
                { L"Signature", L"adsEnabled:!0"},
                { L"Value", L"1"},
                { L"Offset", 12},
                { L"Fill", 0},
                { L"Address", 0}
            }},
            {L"sponsorship", {
                {L"Signature", L".set(\"allSponsorships\",t.sponsorships)}}(e,t);" },
                {L"Value", L"\""},
                {L"Offset", 5},
                {L"Fill", 15},
                {L"Address", 0}
            }},
            {L"skipsentry", {
                {L"Signature", L"sentry.io"},
                {L"Value", L"localhost"},
                {L"Offset", 0},
                {L"Fill", 0},
                {L"Address", 0}
            }},
            {L"hptoEnabled", {
                {L"Signature", L"hptoEnabled:!0"},
                {L"Value", L"1"},
                {L"Offset", 13},
                {L"Fill", 0},
                {L"Address", 0}
            }},
            {L"ishptohidden", {
                {L"Signature", L"isHptoHidden:!0"},
                {L"Value", L"1"},
                {L"Offset", 14},
                {L"Fill", 0},
                {L"Address", 0}
            }},
            {L"sp_localhost", {
                {L"Signature", L"sp://ads/v1/ads/"},
                {L"Value", L"sp://localhost//"},
                {L"Offset", 0},
                {L"Fill", 0},
                {L"Address", 0}
            }},
            {L"premium_free", {
                { L"Signature", L"e.session?.productState?.catalogue?.toLowerCase()" },
                { L"Value", L"\"\""},
                { L"Offset", -1},
                { L"Fill", 48},
                { L"Address", 0}
            }}
        }}
    };

    m_developer = {
        {L"x64", {
            {L"Signature", L"80 E3 01 48 8B 95 ?? ?? ?? ?? 48 83 FA 10"},
            {L"Value", L"B3 01 90"},
            {L"Offset", 0},
            {L"Address", 0}
        }},
        {L"x32", {
            {L"Signature", L"25 01 FF FF FF 89 ?? ?? ?? FF FF"},
            {L"Value", L"B8 03 00"},
            {L"Offset", 0},
            {L"Address", 0}
        }}
    };

    m_cef_offsets = {
        {L"x64", {
            {L"cef_request_t_get_url", 48},
            {L"cef_zip_reader_t_get_file_name", 72},
            {L"cef_zip_reader_t_read_file", 112},
        }},
        {L"x32", {
            {L"cef_request_t_get_url", 24},
            {L"cef_zip_reader_t_get_file_name", 36},
            {L"cef_zip_reader_t_read_file", 56},
        }}
    };

    m_cef_offsets.at(m_architecture).at(L"cef_request_t_get_url").get_to(m_cef_request_t_get_url_offset);
    m_cef_offsets.at(m_architecture).at(L"cef_zip_reader_t_get_file_name").get_to(m_cef_zip_reader_t_get_file_name_offset);
    m_cef_offsets.at(m_architecture).at(L"cef_zip_reader_t_read_file").get_to(m_cef_zip_reader_t_read_file_offset);

    m_app_settings = {
        {L"Latest Release Date", m_latest_release_date},
        {L"Block List", m_block_list},
        {L"Zip Reader", m_zip_reader},
        {L"Developer", m_developer},
        {L"Cef Offsets", m_cef_offsets}
    };

    if (!Utils::WriteFile(m_app_settings_file, m_app_settings.dump(4))) {
        Log(Utils::FormatString(L"Failed to open settings file: {}", m_app_settings_file), LogLevel::Error);
        return false;
    }

    return true;
}

bool SettingsManager::Load()
{
    std::wstring buffer;
    if (!Utils::ReadFile(m_app_settings_file, buffer)) {
        return false;
    }

    m_app_settings = Json::parse(buffer);

    if (!ValidateSettings(m_app_settings)) {
        return false;
    }

    m_app_settings.at(L"Latest Release Date").get_to(m_latest_release_date);
    m_app_settings.at(L"Block List").get_to(m_block_list);
    m_app_settings.at(L"Zip Reader").get_to(m_zip_reader);
    m_app_settings.at(L"Developer").get_to(m_developer);
    m_app_settings.at(L"Cef Offsets").get_to(m_cef_offsets);

    m_app_settings.at(L"Cef Offsets").at(m_architecture).at(L"cef_request_t_get_url").get_to(m_cef_request_t_get_url_offset);
    m_app_settings.at(L"Cef Offsets").at(m_architecture).at(L"cef_zip_reader_t_get_file_name").get_to(m_cef_zip_reader_t_get_file_name_offset);
    m_app_settings.at(L"Cef Offsets").at(m_architecture).at(L"cef_zip_reader_t_read_file").get_to(m_cef_zip_reader_t_read_file_offset);

    if (!m_cef_request_t_get_url_offset || !m_cef_zip_reader_t_get_file_name_offset || !m_cef_zip_reader_t_read_file_offset) {
        Log(L"Failed to load cef offsets from settings file.", LogLevel::Error);
        return false;
    }

    return true;
}

DWORD WINAPI SettingsManager::Update(LPVOID lpParam)
{
    const auto end_time = std::chrono::steady_clock::now() + std::chrono::minutes(1);
    while (std::chrono::steady_clock::now() < end_time) {
        m_settings_changed = (
            m_app_settings.at(L"Latest Release Date") != m_latest_release_date ||
            m_app_settings.at(L"Block List") != m_block_list ||
            m_app_settings.at(L"Zip Reader") != m_zip_reader ||
            m_app_settings.at(L"Developer") != m_developer ||
            m_app_settings.at(L"Cef Offsets") != m_cef_offsets
            );

        if (m_settings_changed) {
            m_app_settings.at(L"Latest Release Date") = m_latest_release_date;
            m_app_settings.at(L"Block List") = m_block_list;
            m_app_settings.at(L"Zip Reader") = m_zip_reader;
            m_app_settings.at(L"Developer") = m_developer;
            m_app_settings.at(L"Cef Offsets") = m_cef_offsets;

            if (!Utils::WriteFile(m_app_settings_file, m_app_settings.dump(4))) {
                Log(Utils::FormatString(L"Failed to open settings file: {}", m_app_settings_file), LogLevel::Error);
            }
        }

        if (m_config.at(L"Enable_Auto_Update") && Logger::HasError()) {
            static Json release_info;
            if (release_info.empty()) {
                release_info = Json::parse(Utils::HttpGetRequest(L"https://api.github.com/repos/mrpond/BlockTheSpot/releases/latest"));
                if (!release_info.contains(L"published_at") || !release_info.at(L"published_at").is_string()) {
                    Log(L"Release info is invalid or doesn't contain published_at field.", LogLevel::Error);
                }
                else if (release_info.at(L"published_at").get_string() != m_latest_release_date) {
                    Json remote_app_settings = Json::parse(Utils::HttpGetRequest(L"https://raw.githubusercontent.com/mrpond/BlockTheSpot/master/blockthespot_settings.json"));
                    if (ValidateSettings(remote_app_settings)) {
                        m_app_settings = remote_app_settings;
                        m_app_settings.at(L"Latest Release Date") = release_info.at(L"published_at").get_string();

                        m_app_settings.at(L"Latest Release Date").get_to(m_latest_release_date);
                        m_app_settings.at(L"Block List").get_to(m_block_list);
                        m_app_settings.at(L"Zip Reader").get_to(m_zip_reader);
                        m_app_settings.at(L"Developer").get_to(m_developer);
                        m_app_settings.at(L"Cef Offsets").get_to(m_cef_offsets);

                        if (!Utils::WriteFile(m_app_settings_file, m_app_settings.dump(4))) {
                            Log(Utils::FormatString(L"Failed to open settings file: {}", m_app_settings_file), LogLevel::Error);
                        }
                        //else if (MessageBoxW(NULL, L"A new version of BlockTheSpot is available! To apply the update, the program needs to be restarted. Would you like to restart now?", L"BlockTheSpot Update Available", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                        //    wchar_t exe_path[MAX_PATH];
                        //    GetModuleFileNameW(NULL, exe_path, MAX_PATH);
                        //    _wsystem(Utils::FormatString(L"powershell.exe -Command \"Stop-Process -Name Spotify; Start-Process -FilePath \"{}\"\"", exe_path).c_str());
                        //}
                    }
                    else {
                        Log(L"Failed to parse app settings from URL.", LogLevel::Error);
                    }
                }
                else {
                    Log(L"No new version of BlockTheSpot available.", LogLevel::Info);
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(20));
    }

    return 0;
}

bool SettingsManager::ValidateSettings(const Json& settings)
{
    // App Settings
    if (settings.empty() || !settings.is_object()) {
        Log(L"Invalid JSON format in settings file.", LogLevel::Error);
        return false;
    }

    const std::vector<std::wstring> keys = { L"Latest Release Date", L"Block List", L"Zip Reader", L"Developer", L"Cef Offsets" };
    for (const auto& key : keys) {
        if (!settings.contains(key)) {
            Log(L"Key '" + key + L"' is missing in settings file.", LogLevel::Error);
            return false;
        }
    }

    if (!settings.at(L"Latest Release Date").is_string() ||
        !settings.at(L"Block List").is_array() ||
        !settings.at(L"Zip Reader").is_object() ||
        !settings.at(L"Developer").is_object() ||
        !settings.at(L"Cef Offsets").is_object()) {
        Log(L"Invalid data types in settings file.", LogLevel::Error);
        return false;
    }

    // Block List
    for (const auto& item : settings.at(L"Block List").get_array()) {
        if (!item.is_string()) {
            Log(L"Invalid data type in Block List.", LogLevel::Error);
            return false;
        }
    }

    // Cef Offsets
    for (const auto& [arch, offset_data] : settings.at(L"Cef Offsets")) {
        if (arch != L"x64" && arch != L"x32") {
            Log(L"Invalid architecture in Cef Offsets settings.", LogLevel::Error);
            return false;
        }

        if (!offset_data.contains(L"cef_request_t_get_url") || !offset_data.at(L"cef_request_t_get_url").is_integer() ||
            !offset_data.contains(L"cef_zip_reader_t_get_file_name") || !offset_data.at(L"cef_zip_reader_t_get_file_name").is_integer() ||
            !offset_data.contains(L"cef_zip_reader_t_read_file") || !offset_data.at(L"cef_zip_reader_t_read_file").is_integer()) {
            Log(L"Invalid data for Cef Offsets in settings file.", LogLevel::Error);
            return false;
        }
    }

    // Developer
    for (const auto& [arch, dev_data] : settings.at(L"Developer")) {
        if (arch != L"x64" && arch != L"x32") {
            Log(L"Invalid architecture in Developer settings.", LogLevel::Error);
            return false;
        }

        if (!dev_data.contains(L"Signature") || !dev_data.at(L"Signature").is_string() ||
            !dev_data.contains(L"Value") || !dev_data.at(L"Value").is_string() ||
            !dev_data.contains(L"Offset") || !dev_data.at(L"Offset").is_integer() ||
            !dev_data.contains(L"Address") || !dev_data.at(L"Address").is_integer()) {
            Log(L"Invalid data for Developer settings in settings file.", LogLevel::Error);
            return false;
        }
    }

    // Zip Reader
    for (const auto& [file_name, file_data] : settings.at(L"Zip Reader")) {
        if (file_name.empty()) {
            Log(L"File name is empty for a Zip Reader entry in settings file.", LogLevel::Error);
            return false;
        }

        if (!file_data.is_object()) {
            Log(L"Invalid data for Zip Reader entry '" + file_name + L"' in settings file.", LogLevel::Error);
            return false;
        }

        for (const auto& [setting_name, setting_data] : file_data) {
            if (setting_name.empty()) {
                Log(L"Setting name is empty for a setting in Zip Reader entry '" + file_name + L"' in settings file.", LogLevel::Error);
                return false;
            }

            if (!setting_data.contains(L"Signature") || !setting_data.at(L"Signature").is_string() ||
                !setting_data.contains(L"Value") || !setting_data.at(L"Value").is_string() ||
                !setting_data.contains(L"Offset") || !setting_data.at(L"Offset").is_integer() ||
                !setting_data.contains(L"Fill") || !setting_data.at(L"Fill").is_integer() ||
                !setting_data.contains(L"Address") || !setting_data.at(L"Address").is_integer()) {
                Log(L"Invalid data for setting '" + setting_name + L"' in Zip Reader entry '" + file_name + L"' in settings file.", LogLevel::Error);
                return false;
            }
        }
    }

    return true;
}

void SettingsManager::SyncConfigFile()
{
    std::wstring ini_path = L".\\config.ini";
    m_config = {
        {L"Block_Ads", true},
        {L"Block_Banner", true},
        {L"Enable_Developer", true},
        {L"Enable_Log", false},
        {L"Enable_Auto_Update", true},
    };

    for (const auto& [key, value] : m_config) {
        std::wstring current_value = Utils::ReadIniFile(ini_path, L"Config", key);
        if (current_value.empty() || (current_value != L"1" && current_value != L"0")) {
            Utils::WriteIniFile(ini_path, L"Config", key, value ? L"1" : L"0");
        }
        else {
            m_config.at(key) = (current_value == L"1");
        }
        PrintStatus(m_config.at(key), key);
    }
}

std::vector<std::wstring> SettingsManager::m_block_list;
Json SettingsManager::m_zip_reader;
Json SettingsManager::m_developer;
Json SettingsManager::m_cef_offsets;

Json SettingsManager::m_app_settings;
std::wstring SettingsManager::m_latest_release_date;
std::wstring SettingsManager::m_app_settings_file;
bool SettingsManager::m_settings_changed;
std::unordered_map<std::wstring, bool> SettingsManager::m_config;

int SettingsManager::m_cef_request_t_get_url_offset;
int SettingsManager::m_cef_zip_reader_t_get_file_name_offset;
int SettingsManager::m_cef_zip_reader_t_read_file_offset;

#ifdef _WIN64
std::wstring SettingsManager::m_architecture = L"x64";
#else
std::wstring SettingsManager::m_architecture = L"x32";
#endif