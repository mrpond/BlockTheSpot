#pragma once
#include "stdafx.h"
#include <fstream>
#include <string>
#include "Config.h"

extern Config g_Config;
class Logger {

public:
	~Logger () {
		if (m_isEnable) {
			m_log.flush ();
			m_log.close ();
		}
	}
	void setLogfile (const std::string& logFile) {
		m_filename = logFile;
		m_isEnable = g_Config.getConfig ("Log");
		if (m_isEnable) {
			m_log.open (logFile, std::ios::out | std::ios::app);
			m_log << "BlockTheSpot - Build date: " << __TIMESTAMP__ << std::endl;
		}
	}

	void Log (const std::string& log) {
		if (m_isEnable)
			m_log << log << std::endl;
	}

private:
	std::string m_filename;
	bool m_isEnable;
	std::ofstream m_log;

};
