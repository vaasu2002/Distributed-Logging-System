
#include <sstream>
#include <iomanip>
#include <regex>

#include "Log.h"

Log::Log(LogLevel level, int subSystemId, const std::string& message, const std::string& source)
	: m_Level(level),
	m_SubSystemId(subSystemId),
	m_Message(message),
	m_Source(source),
	m_TimeStamp(std::chrono::system_clock::now()) {}


LogLevel Log::getLogLevel() const {
	return m_Level;
}

std::string Log::getMessage() const {
	return m_Message;
}

std::string Log::getSource() const {
	return m_Source;
}

std::chrono::system_clock::time_point Log::getTimePoint() const {
	return m_TimeStamp;
}


std::string Log::getTimeStamp() {
    if (m_TimeStampStr.empty()) {
        m_TimeStampStr = formatTimestamp();
    }
    return m_TimeStampStr;
}

// Cross-platform thread-safe localtime (recommended by compiler)
static inline void safe_localtime(std::tm* out, const std::time_t* in) {
#ifdef _WIN32
	localtime_s(out, in);
#else
	localtime_r(in, out);
#endif
}

std::string Log::formatTimestamp() const {
	auto tt = std::chrono::system_clock::to_time_t(m_TimeStamp);
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(m_TimeStamp.time_since_epoch()) % 1000;

	std::tm tm{};
	safe_localtime(&tm, &tt);

	std::ostringstream oss;
	oss << std::put_time(&tm, "%H:%M:%S")
		<< '.' << std::setfill('0') << std::setw(3) << ms.count();

	return oss.str();
}

std::string Log::toString() const {
	std::ostringstream oss;
	oss << "[" << formatTimestamp() << "]"
		<< "[" << LogLevelHelper::logLevelToString(m_Level) << "]"
		<< "[" << m_SubSystemId << "]"
		<< "(" << m_Source << ") "
		<< m_Message;
	return oss.str();
}

// Converts from Log to String
Log Log::parseFromString(const std::string& logLine) {
	std::regex pattern(R"(\[(.*?)\]\[(.*?)\]\[(\d+)\]\((.*?)\)\s(.*))");
	std::smatch match;
	if (!std::regex_match(logLine, match, pattern) || match.size() != 6) {
		throw std::invalid_argument("Invalid log format: " + logLine);
	}

	std::string timeStr = match[1];
	std::string levelStr = match[2];
	int subsystemId = std::stoi(match[3]);
	std::string source = match[4];
	std::string message = match[5];

	LogLevel level = LogLevelHelper::StringToLogLevel(levelStr);
	Log log(level, subsystemId, message, source);
	log.m_TimeStampStr = timeStr;

	// Parse timestamp string - handle potential negative milliseconds
	std::tm tm = {};
	int hour, min, sec, ms;

	// Initialize tm structure with current date to avoid garbage values
	auto now = std::chrono::system_clock::now();
	auto tt_now = std::chrono::system_clock::to_time_t(now);
	safe_localtime(&tm, &tt_now);

	if (sscanf_s(timeStr.c_str(), "%d:%d:%d.%d", &hour, &min, &sec, &ms) == 4) {
		// Validate and clamp values
		if (hour < 0 || hour > 23) hour = 0;
		if (min < 0 || min > 59) min = 0;
		if (sec < 0 || sec > 59) sec = 0;
		if (ms < 0) ms = 0; // Clamp negative milliseconds to 0
		if (ms > 999) ms = 999;

		tm.tm_hour = hour;
		tm.tm_min = min;
		tm.tm_sec = sec;

		std::time_t tt = std::mktime(&tm);
		log.m_TimeStamp = std::chrono::system_clock::from_time_t(tt) +
			std::chrono::milliseconds(ms);
	}
	else {
		// If parsing fails, use current time
		log.m_TimeStamp = std::chrono::system_clock::now();
	}

	return log;
}
std::chrono::system_clock::time_point Log::getTimestamp() const {
	return m_TimeStamp;
}