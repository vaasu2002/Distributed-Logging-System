#include "LogLevel.h"

int LogLevelHelper::getLogLevel(LogLevel level) {
	return static_cast<int>(level);
}

std::string LogLevelHelper::logLevelToString(LogLevel level) {
	switch (level) {
	case LogLevel::DEBUG:
		return "DEBUG";
	case LogLevel::INFO:
		return "INFO";
	case LogLevel::ERRORS:
		return "ERROR";
	case LogLevel::WARN:
		return "WARN";
	case LogLevel::FATAL:
		return "FATAL";
	default:
		throw std::invalid_argument("Unknown log level");
	}
}

LogLevel LogLevelHelper::StringToLogLevel(const std::string& level) {
	if (level == "DEBUG") {
		return LogLevel::DEBUG;
	}
	else if (level == "INFO") {
		return LogLevel::INFO;
	}
	else if (level == "WARN") {
		return LogLevel::WARN;
	}
	else if (level == "ERROR") {
		return LogLevel::ERRORS;
	}
	else if (level == "FATAL") {
		return LogLevel::FATAL;
	}
	else {
		throw std::invalid_argument("Unknown log level string provided: " + level);
	}
}