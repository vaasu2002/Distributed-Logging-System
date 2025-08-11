#pragma once
#include "ISink.h"

/**
 * @class ConsoleSink
 * @brief Writes logs to console (stdout)
 */
class ConsoleSink : public ISink {
private:
	mutable std::mutex m_Mutex;

public:
	void write(const std::string& message) override {
		std::lock_guard<std::mutex> lock(m_Mutex);
		std::cout << message << std::endl;
	}

	void flush() override {
		std::lock_guard<std::mutex> lock(m_Mutex);
		std::cout.flush();
	}

	bool isReady() const override {
		return true; 
	}
};