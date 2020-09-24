#pragma once
#include <string>
#include <map>

class Config {
	
public:
	Config () {
		m_filename = "./config.ini";
		m_data["Log"] = read ("Config", "Log");
		m_data["Skip_wpad"] = read ("Config", "Skip_wpad");
		m_data["Block_BannerOnly"] = read ("Config", "Block_BannerOnly");
	}

	bool getConfig (const char* key) {
		auto find = m_data.find (key);
		if (find != m_data.end()) {
			return find->second;
		}
		return false;
	}
private:
	std::string m_filename;
	std::map<std::string, bool> m_data;

	bool read (const char* app, const char* key, int def_value = 0) {
		if (1 == GetPrivateProfileInt (app, key, def_value, m_filename.c_str ())) {
			return true;
		}
		return false;
	}
};

