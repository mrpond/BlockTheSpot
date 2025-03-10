#include "pch.h"

#include <chrono>
#include <thread>
#include <fstream>
#include <filesystem>

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
    // Removed hardcoded date

    m_block_list = {
        L"/ads/",
        L"/ad-logic/",
        L"/gabo-receiver-service/"
    };

    m_app_settings[L"Latest Release Date"] = m_latest_release_date; // Save the current date
    m_app_settings[L"Block List"] = m_block_list;
    m_app_settings[L"Zip Reader"] = m_zip_reader;
    m_app_settings[L"Developer"] = m_developer;
    m_app_settings[L"Cef Offsets"] = m_cef_offsets;

    if (!Utils::WriteFile(m_app_settings_file, m_app_settings.dump(2))) {
        LogError(L"Failed to save settings to file: {}", m_app_settings_file);
        return false;
    }
    return true;
}

bool SettingsManager::Load(const Json& settings)
{
    if (settings == nullptr) {
        std::wstring buffer;
        if (!Utils::ReadFile(m_app_settings_file, buffer)) {
            LogError(L"Failed to read settings file: {}", m_app_settings_file);
            return false;
        }

        try {
            m_app_settings = Json::parse(buffer);
        }
        catch (const std::exception& ex) {
            LogError(L"Failed to parse settings file: {}. Error: {}", m_app_settings_file, Utils::ToString(ex.what()));
            return false;
        }

        if (!ValidateSettings(m_app_settings)) {
            LogError(L"Settings validation failed after loading from file.");
            return false;
        }
    }
    else {
        m_app_settings = settings;
    }

    try {
        m_latest_release_date = m_app_settings.at(L"Latest Release Date").get_string();

        // Manually convert JSON array to vector<wstring>
        auto blockListArray = m_app_settings.at(L"Block List");
        m_block_list.clear();
        for (size_t i = 0; i < blockListArray.size(); i++) {
            m_block_list.push_back(blockListArray[i].get_string());
        }

        m_zip_reader = m_app_settings.at(L"Zip Reader");
        m_developer = m_app_settings.at(L"Developer");
        m_cef_offsets = m_app_settings.at(L"Cef Offsets");
    }
    catch (const std::exception& ex) {
        LogError(L"Failed to retrieve settings from JSON. Error: {}", Utils::ToString(ex.what()));
        return false;
    }

    if (!m_cef_request_t_get_url_offset || !m_cef_zip_reader_t_get_file_name_offset || !m_cef_zip_reader_t_read_file_offset) {
        m_cef_request_t_get_url_offset = m_cef_offsets.at(L"x64").at(L"cef_request_t_get_url").get_integer();
        m_cef_zip_reader_t_get_file_name_offset = m_cef_offsets.at(L"x64").at(L"cef_zip_reader_t_get_file_name").get_integer();
        m_cef_zip_reader_t_read_file_offset = m_cef_offsets.at(L"x64").at(L"cef_zip_reader_t_read_file").get_integer();
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
    try {
        const auto server_settings_string = Utils::HttpGetRequest(L"https://raw.githubusercontent.com/mrpond/BlockTheSpot/master/blockthespot_settings.json");
        const auto server_settings = Json::parse(server_settings_string);

        if (!ValidateSettings(server_settings)) {
            LogError(L"Server settings validation failed.");
            return false;
        }

        if (!CompareSettings(server_settings)) {
            const auto server_release_date = server_settings.at(L"Latest Release Date").get_string();
            const auto forced_update = m_latest_release_date != server_release_date;

            if (!Load(server_settings) || !Utils::WriteFile(m_app_settings_file, server_settings.dump(2))) {
                LogError(L"Failed to load server settings or write to the settings file: {}", m_app_settings_file);
                return false;
            }
            else {
                LogInfo(L"Settings updated from server.");
                m_latest_release_date = server_release_date; // Update local date
            }

            if (forced_update && MessageBoxW(NULL, L"A new version of BlockTheSpot is available. Do you want to update?", L"BlockTheSpot Update Available", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                // Safer update mechanism: download a zip, extract, and replace files.
                // TODO: Implement the zip download and extraction logic here.
                // For now, just log a message.
                LogInfo(L"Update requested by user.  Implement ZIP download/extraction here.");

                // Example of a safer update:
                // 1. Download the zip file from a URL.
                // 2. Extract the contents to a temporary directory.
                // 3. Replace the necessary files in the application directory.
                // 4. Clean up the temporary directory.

                //_wsystem(L"powershell -Command \"& {[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -UseBasicParsing 'https://raw.githubusercontent.com/mrpond/BlockTheSpot/master/install.ps1' | Invoke-Expression}\"");
            }
        }
        return true;
    }
    catch (const std::exception& e) {
        LogError(L"Error updating settings from server: {}", Utils::ToString(e.what()));
        return false;
    }
}

bool SettingsManager::ValidateSettings(const Json& settings)
{
    if (settings.empty() || !settings.is_object()) {
        LogError(L"Settings are empty or not an object.");
        return false;
    }

    // Add more validation checks here to ensure the settings are valid.
    if (!settings.contains(L"Latest Release Date") || !settings.at(L"Latest Release Date").is_string()) {
        LogError(L"Missing or invalid 'Latest Release Date' setting.");
        return false;
    }

    if (!settings.contains(L"Block List") || !settings.at(L"Block List").is_array()) {
        LogError(L"Missing or invalid 'Block List' setting.");
        return false;
    }

    if (!settings.contains(L"Zip Reader") || !settings.at(L"Zip Reader").is_object()) {
        LogError(L"Missing or invalid 'Zip Reader' setting.");
        return false;
    }

    if (!settings.contains(L"Developer") || !settings.at(L"Developer").is_object()) {
        LogError(L"Missing or invalid 'Developer' setting.");
        return false;
    }

    if (!settings.contains(L"Cef Offsets") || !settings.at(L"Cef Offsets").is_object()) {
        LogError(L"Missing or invalid 'Cef Offsets' setting.");
        return false;
    }

    return true;
}

bool SettingsManager::CompareSettings(const Json& current_settings, const Json& reference_settings)
{
    return current_settings == reference_settings;
}

void SettingsManager::SyncConfigFile()
{
    std::wifstream configFile("config.ini");
    if (!configFile.is_open()) {
        m_config[L"Block_Ads"] = true;
        m_config[L"Block_Banner"] = true;
        m_config[L"Enable_Developer"] = true;
        m_config[L"Enable_Auto_Update"] = true;
        m_config[L"Enable_Log"] = false;
        return;
    }

    std::wstring line;
    while (getline(configFile, line)) {
        if (line.find(L"Block_Ads=") != std::wstring::npos) {
            m_config[L"Block_Ads"] = (line.substr(line.find(L"=") + 1) == L"1");
        }
        else if (line.find(L"Block_Banner=") != std::wstring::npos) {
            m_config[L"Block_Banner"] = (line.substr(line.find(L"=") + 1) == L"1");
        }
        else if (line.find(L"Enable_Developer=") != std::wstring::npos) {
            m_config[L"Enable_Developer"] = (line.substr(line.find(L"=") + 1) == L"1");
        }
        else if (line.find(L"Enable_Auto_Update=") != std::wstring::npos) {
            m_config[L"Enable_Auto_Update"] = (line.substr(line.find(L"=") + 1) == L"1");
        }
        else if (line.find(L"Enable_Log=") != std::wstring::npos) {
            m_config[L"Enable_Log"] = (line.substr(line.find(L"=") + 1) == L"1");
        }
    }
    configFile.close();
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