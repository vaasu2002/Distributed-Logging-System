#include "ConsoleSink.h"
#include "FileSink.h"

namespace SinkFactory {
	inline std::unique_ptr<ISink> createConsoleSink() {
		return std::make_unique<ConsoleSink>();
	}

	inline std::unique_ptr<ISink> createFileSink(const std::string& filename, bool autoFlush = false, bool append = true) {
		return std::make_unique<FileSink>(filename, autoFlush, append);
	}
}