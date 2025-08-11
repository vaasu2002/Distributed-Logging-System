#pragma once
#include <string>
#include <vector>

class ConfigEditor {
public:
	ConfigEditor(const std::string& configPath);
	bool updateAppConfig(int appId,
		const std::vector<std::string>& filters,
		const std::vector<std::string>& appenders);
	bool getAppConfig(int appId,
		std::vector<std::string>& filtersOut,
		std::vector<std::string>& appendersOut);
private:
	std::string m_FilePath;

	bool lockFile(void*& fileHandle);
	void unlockFile(void* fileHandle);
	std::string readFile(void* fileHandle);
	void writeFile(void* fileHandle, const std::string& content);

	std::vector<std::string> parseJsonArray(const std::string& json);
	bool containsApp(const std::string& entry, int appId);
};
