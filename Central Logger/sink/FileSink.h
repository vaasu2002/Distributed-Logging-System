#include "ISink.h"

/**
 * @class FileSink
 * @brief Writes logs to a file with optional rotation
 */
class FileSink : public ISink {
private:
	std::ofstream m_File;
	std::string m_Filename;
	mutable std::mutex m_Mutex;
	bool m_AutoFlush;

public:
	/**
	 * @brief Constructs a FileSink
	 * @param filename Path to the log file
	 * @param autoFlush Whether to flush after each write (default: false)
	 * @param append Whether to append to existing file (default: true)
	 */
	explicit FileSink(const std::string& filename, bool autoFlush = false, bool append = true)
		: m_Filename(filename), m_AutoFlush(autoFlush) {
		std::ios::openmode mode = std::ios::out;
		if (append) {
			mode |= std::ios::app;
		}
		m_File.open(filename, mode);
	}

	~FileSink() {
		std::lock_guard<std::mutex> lock(m_Mutex);
		if (m_File.is_open()) {
			m_File.close();
		}
	}

	void write(const std::string& message) override {
		std::lock_guard<std::mutex> lock(m_Mutex);
		if (m_File.is_open()) {
			m_File << message << std::endl;
			if (m_AutoFlush) {
				m_File.flush();
			}
		}
	}

	void flush() override {
		std::lock_guard<std::mutex> lock(m_Mutex);
		if (m_File.is_open()) {
			m_File.flush();
		}
	}

	bool isReady() const override {
		std::lock_guard<std::mutex> lock(m_Mutex);
		return m_File.is_open() && m_File.good();
	}

	const std::string& getFilename() const { return m_Filename; }
};