#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

class SettingsManager {
public:
    static void Init();

    static std::unordered_map<std::wstring, bool> m_config;
    static std::vector<std::wstring> m_block_list;
    static Json m_developer;
    static Json m_zip_reader;
    static Json m_cef_offsets;

    static int m_cef_request_t_get_url_offset;
    static int m_cef_zip_reader_t_get_file_name_offset;
    static int m_cef_zip_reader_t_read_file_offset;

    static std::wstring m_architecture;

private:
    static bool Save();
    static bool Load();
    static DWORD WINAPI Update(LPVOID lpParam);
    static bool ValidateSettings(const Json& settings);
    static void SyncConfigFile();

    static Json m_app_settings;
    static std::wstring m_latest_release_date;
    static std::wstring m_app_settings_file;

    static bool m_settings_changed;
};

#endif // SETTINGS_MANAGER_H
