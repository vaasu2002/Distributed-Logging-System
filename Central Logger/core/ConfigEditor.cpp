#include "ConfigEditor.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <windows.h>
#include <algorithm>

ConfigEditor::ConfigEditor(const std::string& configPath) : m_FilePath(configPath) {}

bool ConfigEditor::lockFile(void*& handleOut) {
	static HANDLE mutex = CreateMutexA(NULL, FALSE, "ConfigEditorMutex");
	if (WaitForSingleObject(mutex, 3000) != WAIT_OBJECT_0) {
		return false;
	}
	HANDLE fileHandle = CreateFileA(m_FilePath.c_str(),
		GENERIC_READ | GENERIC_WRITE,
		0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	handleOut = fileHandle;
	return fileHandle != INVALID_HANDLE_VALUE;
}

void ConfigEditor::unlockFile(void* handle) {
	if (handle) CloseHandle((HANDLE)handle);
	static HANDLE mutex = CreateMutexA(NULL, FALSE, "ConfigEditorMutex");
	ReleaseMutex(mutex);
}

std::string ConfigEditor::readFile(void* handle) {
	HANDLE fileHandle = (HANDLE)handle;
	DWORD fileSize = GetFileSize(fileHandle, NULL);
	if (fileSize == 0) return "[]";

	std::vector<char> buffer(fileSize + 1);
	DWORD bytesRead;
	ReadFile(fileHandle, buffer.data(), fileSize, &bytesRead, NULL);
	buffer[bytesRead] = '\0';
	return std::string(buffer.data());
}

void ConfigEditor::writeFile(void* handle, const std::string& content) {
	HANDLE fileHandle = (HANDLE)handle;
	SetFilePointer(fileHandle, 0, NULL, FILE_BEGIN);
	SetEndOfFile(fileHandle);
	DWORD bytesWritten;
	WriteFile(fileHandle, content.c_str(), content.length(), &bytesWritten, NULL);
}

std::vector<std::string> ConfigEditor::parseJsonArray(const std::string& json) {
	std::vector<std::string> entries;
	size_t pos = 0;
	int braceCount = 0;
	size_t objectStart = 0;
	bool inObject = false;

	for (size_t i = 0; i < json.length(); i++) {
		if (json[i] == '{') {
			if (!inObject) {
				objectStart = i;
				inObject = true;
			}
			braceCount++;
		}
		else if (json[i] == '}') {
			braceCount--;
			if (braceCount == 0 && inObject) {
				entries.push_back(json.substr(objectStart, i - objectStart + 1));
				inObject = false;
			}
		}
	}

	return entries;
}

bool ConfigEditor::containsApp(const std::string& entry, int appId) {
	std::string key = "\"app_" + std::to_string(appId) + "\"";
	return entry.find(key) != std::string::npos;
}

bool ConfigEditor::updateAppConfig(int appId,
	const std::vector<std::string>& filters,
	const std::vector<std::string>& appenders) {
	void* fileHandle;
	std::string originalContent = "[]";

	if (!lockFile(fileHandle)) return false;

	originalContent = readFile(fileHandle);
	if (originalContent.empty()) originalContent = "[]";

	unlockFile(fileHandle);

	std::vector<std::string> entries = parseJsonArray(originalContent);
	std::ostringstream newEntry;

	newEntry << "  {\n";
	newEntry << "    \"app_" << appId << "\": {\n";

	newEntry << "      \"filters\": [\n";
	for (size_t i = 0; i < filters.size(); ++i) {
		newEntry << "        \"" << filters[i] << "\"";
		if (i < filters.size() - 1) newEntry << ",";
		newEntry << "\n";
	}
	newEntry << "      ],\n";

	newEntry << "      \"appenders\": [\n";
	for (size_t i = 0; i < appenders.size(); ++i) {
		newEntry << "        \"" << appenders[i] << "\"";
		if (i < appenders.size() - 1) newEntry << ",";
		newEntry << "\n";
	}
	newEntry << "      ]\n";
	newEntry << "    }\n";
	newEntry << "  }";

	bool found = false;
	for (size_t i = 0; i < entries.size(); ++i) {
		if (containsApp(entries[i], appId)) {
			entries[i] = newEntry.str();
			found = true;
			break;
		}
	}

	if (!found) entries.push_back(newEntry.str());

	std::ostringstream finalJson;
	finalJson << "[\n";
	for (size_t i = 0; i < entries.size(); ++i) {
		finalJson << entries[i];
		if (i < entries.size() - 1) finalJson << ",";
		finalJson << "\n";
	}
	finalJson << "]";

	if (!lockFile(fileHandle)) return false;
	writeFile(fileHandle, finalJson.str());
	unlockFile(fileHandle);

	return true;
}

bool ConfigEditor::getAppConfig(int appId,
	std::vector<std::string>& filtersOut,
	std::vector<std::string>& appendersOut) {
	void* fileHandle;
	if (!lockFile(fileHandle)) return false;

	std::string content = readFile(fileHandle);
	unlockFile(fileHandle);
	if (content.empty()) return false;

	std::vector<std::string> entries = parseJsonArray(content);
	std::string key = "\"app_" + std::to_string(appId) + "\"";

	for (const auto& entry : entries) {
		if (entry.find(key) != std::string::npos) {
			// extract filters
			size_t fStart = entry.find("\"filters\"");
			if (fStart != std::string::npos) {
				size_t arrStart = entry.find("[", fStart);
				size_t arrEnd = entry.find("]", arrStart);
				std::string fArray = entry.substr(arrStart + 1, arrEnd - arrStart - 1);
				std::stringstream ss(fArray);
				std::string val;
				while (std::getline(ss, val, ',')) {
					val.erase(remove(val.begin(), val.end(), '"'), val.end());
					val.erase(remove_if(val.begin(), val.end(), ::isspace), val.end());
					if (!val.empty()) filtersOut.push_back(val);
				}
			}

			// extract appenders
			size_t aStart = entry.find("\"appenders\"");
			if (aStart != std::string::npos) {
				size_t arrStart = entry.find("[", aStart);
				size_t arrEnd = entry.find("]", arrStart);
				std::string aArray = entry.substr(arrStart + 1, arrEnd - arrStart - 1);
				std::stringstream ss(aArray);
				std::string val;
				while (std::getline(ss, val, ',')) {
					val.erase(remove(val.begin(), val.end(), '"'), val.end());
					val.erase(remove_if(val.begin(), val.end(), ::isspace), val.end());
					if (!val.empty()) appendersOut.push_back(val);
				}
			}

			return true;
		}
	}

	return false;
}