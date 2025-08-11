#pragma once
#ifndef LOG_H

#include "LogLevel.h"
#include <string>
#include <chrono>

class Log {
	LogLevel m_Level;
	int m_SubSystemId;
	std::string m_Message;
	std::string m_Source;
	// variables related to timestamp
	std::chrono::system_clock::time_point m_TimeStamp;
	mutable std::string m_TimeStampStr;
public:
	Log() = default;
	Log(LogLevel level, int subSystemId, const std::string& message, const std::string& source);

	LogLevel getLogLevel() const;
	std::string getMessage() const;
	std::string getSource() const;
	std::chrono::system_clock::time_point getTimePoint() const;

	std::string toString() const;
	std::string getTimeStamp();

	std::string formatTimestamp() const;
	static Log parseFromString(const std::string& logLine);
	std::chrono::system_clock::time_point getTimestamp() const;
};

#endif // !LOG_H