#pragma once
#ifndef LOGLEVEL_H
#define LOGLEVEL_H

#include <string>

enum class LogLevel {
	DEBUG = 1,
	INFO = 2,
	ERRORS = 3,
	WARN = 4,
	FATAL = 5
};

class LogLevelHelper {
public:
	static int getLogLevel(LogLevel level);
	static std::string logLevelToString(LogLevel level);
	static LogLevel StringToLogLevel(const std::string& level);
};

#endif LOGLEVEL_H