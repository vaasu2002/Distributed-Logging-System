#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>

/**
 * @interface ILogSink
 * @brief Abstract interface for different log output destinations
 */
class ISink {
public:
	virtual ~ISink() = default;

	/**
	 * @brief Write a log message to the sink
	 * @param message The formatted log message to write
	 */
	virtual void write(const std::string& message) = 0;

	/**
	 * @brief Flush any buffered output (optional)
	 */
	virtual void flush() {}

	/**
	 * @brief Check if the sink is ready for writing
	 * @return true if sink is available, false otherwise
	 */
	virtual bool isReady() const = 0;
};