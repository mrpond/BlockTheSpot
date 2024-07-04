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
            LogError(L"Failed to open settings file: {}", m_app_settings_file);
        }
    }

    auto thread = CreateThread(NULL, 0, Update, NULL, 0, NULL);
    if (thread != nullptr) {
        CloseHandle(thread);
    }
}

bool SettingsManager::Save()
{
    m_latest_release_date = L"2024-06-16"; // Update only when significant changes occur.
    
    m_block_list = { 
        L"/ads/",
        L"/ad-logic/",
        L"/gabo-receiver-service/"
    };

    m_zip_reader = {
        {L"home-hpto.css", {
            {L"hptocss", {
                {L"Signature", L".utUDWsORU96S7boXm2Aq{display:-webkit-box;display:-ms-flexbox;display:flex;"},
                {L"Value", L"none"},
                {L"Offset", 70},
                {L"Fill", 0},
                {L"Address", -1}
            }}
        }},
        {L"xpui.js", {
            {L"adsEnabled", {
                {L"Signature", L"adsEnabled:!0"},
                {L"Value", L"1"},
                {L"Offset", 12},
                {L"Fill", 0},
                {L"Address", -1}
            }},
            {L"sponsorship", {
                {L"Signature", L".set(\"allSponsorships\",t.sponsorships)}}(e,t);"},
                {L"Value", L"\""},
                {L"Offset", 5},
                {L"Fill", 15},
                {L"Address", -1}
            }},
            {L"skipsentry", {
                {L"Signature", L"sentry.io"},
                {L"Value", L"localhost"},
                {L"Offset", 0},
                {L"Fill", 0},
                {L"Address", -1}
            }},
            {L"hptoEnabled", {
                {L"Signature", L"hptoEnabled:!0"},
                {L"Value", L"1"},
                {L"Offset", 13},
                {L"Fill", 0},
                {L"Address", -1}
            }},
            {L"ishptohidden", {
                {L"Signature", L"isHptoHidden:!0"},
                {L"Value", L"1"},
                {L"Offset", 14},
                {L"Fill", 0},
                {L"Address", -1}
            }},
            {L"sp_localhost", {
                {L"Signature", L"sp://ads/v1/ads/"},
                {L"Value", L"sp://localhost//"},
                {L"Offset", 0},
                {L"Fill", 0},
                {L"Address", -1}
            }},
            {L"premium_free", {
                {L"Signature", L"\"free\"===e.session?.productState?.catalogue?.toLowerCase()"},
                {L"Value", L"\""},
                {L"Offset", 0},
                {L"Fill", 4},
                {L"Address", -1}
            }}
        }}
    };

    m_developer = {
        {L"x64", {
            {L"Signature", L"80 E3 01 48 8B 95 ?? ?? ?? ?? 48 83 FA 10"},
            {L"Value", L"B3 01 90"},
            {L"Offset", 0},
            {L"Address", -1}
        }},
        {L"x32", {
            {L"Signature", L"25 01 FF FF FF 89 ?? ?? ?? FF FF"},
            {L"Value", L"B8 03 00"},
            {L"Offset", 0},
            {L"Address", -1}
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

    if (!Utils::WriteFile(m_app_settings_file, m_app_settings.dump(2))) {
        LogError(L"Failed to open settings file: {}", m_app_settings_file);
        return false;
    }

    return true;
}

bool SettingsManager::Load(const Json& settings)
{
    if (settings == nullptr) {
        std::wstring buffer;
        if (!Utils::ReadFile(m_app_settings_file, buffer)) {
            return false;
        }

        m_app_settings = Json::parse(buffer);

        if (!ValidateSettings(m_app_settings)) {
            return false;
        }
    }
    else {
        m_app_settings = settings;
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
        LogError(L"Failed to load cef offsets from settings file.");
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
            m_app_settings.at(L"Cef Offsets") != m_cef_offsets);

        if (m_settings_changed) {
            m_app_settings.at(L"Latest Release Date") = m_latest_release_date;
            m_app_settings.at(L"Block List") = m_block_list;
            m_app_settings.at(L"Zip Reader") = m_zip_reader;
            m_app_settings.at(L"Developer") = m_developer;
            m_app_settings.at(L"Cef Offsets") = m_cef_offsets;

            if (!Utils::WriteFile(m_app_settings_file, m_app_settings.dump(2))) {
                LogError(L"Failed to open settings file: {}", m_app_settings_file);
            }
        }

        if (m_config.at(L"Enable_Auto_Update") && Logger::HasError()) {
            static bool update_done;
            if (!update_done) {
                update_done = UpdateSettingsFromServer();
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(20));
    }

    return 0;
}

bool SettingsManager::UpdateSettingsFromServer()
{
    const auto server_settings = Json::parse(Utils::HttpGetRequest(L"https://raw.githubusercontent.com/mrpond/BlockTheSpot/master/blockthespot_settings.json"));

    if (!ValidateSettings(server_settings)) {
        LogError(L"Server settings validation failed.");
        return false;
    }
    
    if (!CompareSettings(server_settings)) {
        const auto forced_update = m_latest_release_date != server_settings.at(L"Latest Release Date");
        
        if (!Load(server_settings) || !Utils::WriteFile(m_app_settings_file, server_settings.dump(2))) {
            LogError(L"Failed to load server settings or write to the settings file: {}", m_app_settings_file);
            return false;
        }
        else {
            LogInfo(L"Settings updated from server.");
        }

        if (forced_update && MessageBoxW(NULL, L"A new version of BlockTheSpot is available. Do you want to update?", L"BlockTheSpot Update Available", MB_YESNO | MB_ICONQUESTION) == IDYES) {
            _wsystem(L"powershell -Command \"& {[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -UseBasicParsing 'https://raw.githubusercontent.com/mrpond/BlockTheSpot/master/install.ps1' | Invoke-Expression}\"");
        }
    }

    return true;
}

bool SettingsManager::ValidateSettings(const Json& settings)
{
    // App Settings
    if (settings.empty() || !settings.is_object()) {
        LogError(L"Invalid JSON format in settings file.");
        return false;
    }

    static const std::unordered_map<std::wstring, Json::ValueType> keys = { 
        {L"Latest Release Date", Json::ValueType::String},
        {L"Block List", Json::ValueType::Array},
        {L"Zip Reader", Json::ValueType::Object},
        {L"Developer", Json::ValueType::Object},
        {L"Cef Offsets", Json::ValueType::Object}
    };

    for (const auto& key : keys) {
        if (!settings.contains(key.first) || settings.at(key.first).type() != key.second) {
            LogError(L"Invalid or missing '{}' in settings file.", key.first);
            return false;
        }
    }

    // Block List
    for (const auto& item : settings.at(L"Block List").get_array()) {
        if (!item.is_string()) {
            LogError(L"Invalid data type in Block List.");
            return false;
        }
    }

    // Cef Offsets
    for (const auto& [arch, offset_data] : settings.at(L"Cef Offsets")) {
        if (arch != L"x64" && arch != L"x32") {
            LogError(L"Invalid architecture in Cef Offsets settings.");
            return false;
        }

        static const std::vector<std::wstring> offset_keys = { L"cef_request_t_get_url", L"cef_zip_reader_t_get_file_name", L"cef_zip_reader_t_read_file" };
        for (const auto& key : offset_keys) {
            if (!offset_data.contains(key) || !offset_data.at(key).is_integer()) {
                LogError(L"Invalid or missing key '{}' in Cef Offsets settings.", key);
                return false;
            }
        }
    }

    // Developer
    for (const auto& [arch, dev_data] : settings.at(L"Developer")) {
        if (arch != L"x64" && arch != L"x32") {
            LogError(L"Invalid architecture in Developer settings.");
            return false;
        }

        static const std::unordered_map<std::wstring, Json::ValueType> dev_keys = { 
            {L"Signature", Json::ValueType::String},
            {L"Value", Json::ValueType::String},
            {L"Offset", Json::ValueType::Integer},
            {L"Address", Json::ValueType::Integer}
        };

        for (const auto& key : dev_keys) {
            if (!dev_data.contains(key.first) || dev_data.at(key.first).type() != key.second) {
                LogError(L"Invalid or missing data for key '{}' in Developer settings.", key.first);
                return false;
            }
        }
    }

    // Zip Reader
    for (const auto& [file_name, file_data] : settings.at(L"Zip Reader")) {
        if (file_name.empty()) {
            LogError(L"File name is empty for a Zip Reader entry in settings file.");
            return false;
        }

        if (!file_data.is_object()) {
            LogError(L"Invalid data for Zip Reader entry '{}' in settings file.", file_name);
            return false;
        }

        for (const auto& [setting_name, setting_data] : file_data) {
            if (setting_name.empty()) {
                LogError(L"Setting name is empty for a setting in Zip Reader entry '{}' in settings file.", file_name);
                return false;
            }

            if (!setting_data.contains(L"Signature") || !setting_data.at(L"Signature").is_string() ||
                !setting_data.contains(L"Value") || !setting_data.at(L"Value").is_string() ||
                !setting_data.contains(L"Offset") || !setting_data.at(L"Offset").is_integer() ||
                !setting_data.contains(L"Fill") || !setting_data.at(L"Fill").is_integer() ||
                !setting_data.contains(L"Address") || !setting_data.at(L"Address").is_integer()) {
                LogError(L"Invalid data for setting '{}' in Zip Reader entry '{}' in settings file.", setting_name, file_name);
                return false;
            }
        }
    }

    return true;
}

bool SettingsManager::CompareSettings(const Json& current_settings, const Json& reference_settings/*, bool update_if_changed*/)
{
    for (const auto& item : current_settings) {
        if (item.first == L"Address") {
            continue;
        }
        if (!reference_settings.contains(item.first)) {
            // if (update_if_changed) {
            //     m_app_settings[item.first] = item.second;
            // }
            return false;
        }
        const auto& app_value = reference_settings.at(item.first);
        if (item.second.is_object()) {
            if (!CompareSettings(item.second, app_value)) {
                // if (update_if_changed) {
                //     m_app_settings[item.first] = item.second;
                // }
                return false;
            }
        }
        else {
            if (item.second != app_value) {
                // if (update_if_changed) {
                //     m_app_settings[item.first] = item.second;
                // }
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