#pragma once
#include <queue>
#include <mutex>
#include <vector>
#include "BaseQueue.h"

/**
* @class LogDLQueue
* @brief Thread-safe dead letter queue that tracks memory usage and handles eviction.
* 
* @details
* - Stores logs that could not be processed in the primary flow (e.g., due to overload).
* - Processed only when the priority queue is not overloaded.
*/
class LogDLQueue : public IBaseQueue {
private:
	std::queue<Log> m_Queue; // Real queue storing logs

	// Variables for thread-safe operations
	mutable std::mutex m_Mutex; // Mutex for thread-safe read and write on priority queue.

	// Variables for tracking size and eviction.
	size_t m_CurrentSizeInBytes = 0;
	const size_t m_MaxSizeInBytes;
	const double m_EvictionThresholdSoft = 0.6; // Threshold for triggering soft evictions
	const double m_EvictionThresholdHard = 0.9;  // Threshold for triggering hard evictions
	const double m_EvictionStopThresholdHard = 0.7;

public:
	explicit LogDLQueue(size_t maxSizeInMB)
		: m_MaxSizeInBytes(maxSizeInMB * 1024 * 1024) {}

	bool enqueue(const Log& log) override {
		std::lock_guard<std::mutex> lock(m_Mutex);
		return enqueueInternal(log);
	}

	bool enqueueBatch(const std::vector<Log>& logs) override {
		std::lock_guard<std::mutex> lock(m_Mutex);
		for (const auto& log : logs) {
			if (!enqueueInternal(log)) {
				// If one log fails due to size, skip it (don't fail entire batch)
				continue;
			}
		}
		return true;
	}

	bool dequeue(Log& outLog) override {
		std::lock_guard<std::mutex> lock(m_Mutex);
		if (m_Queue.empty()) return false;

		outLog = m_Queue.front();
		m_CurrentSizeInBytes -= computeLogSizeInBytes(outLog);
		m_Queue.pop();
		return true;
	}

	size_t dequeueBatch(std::vector<Log>& outLogs, size_t maxCount) override {
		std::lock_guard<std::mutex> lock(m_Mutex);
		size_t count = 0;
		while (count < maxCount && !m_Queue.empty()) {
			Log log = m_Queue.front();
			m_CurrentSizeInBytes -= computeLogSizeInBytes(log);
			m_Queue.pop();
			outLogs.push_back(std::move(log));
			++count;
		}
		return count;
	}

	size_t size() const override {
		std::lock_guard<std::mutex> lock(m_Mutex);
		return m_Queue.size();
	}

	size_t currentSizeInBytes() const {
		std::lock_guard<std::mutex> lock(m_Mutex);
		return m_CurrentSizeInBytes;
	}

	void reset() override {
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_CurrentSizeInBytes = 0;
		while (!m_Queue.empty()) {
			m_Queue.pop();
		}
	}

private:
	bool enqueueInternal(const Log& log) override {
		size_t logSize = computeLogSizeInBytes(log);

		// Reject if the log itself is larger than the max capacity
		if (logSize > m_MaxSizeInBytes) {
			return false;
		}

		// Soft eviction
		if (m_CurrentSizeInBytes > m_MaxSizeInBytes * m_EvictionThresholdSoft) {
			int itemsToEvict = std::min<size_t>(10, m_Queue.size());
			while (itemsToEvict-- > 0 && !m_Queue.empty()) {
				m_CurrentSizeInBytes -= computeLogSizeInBytes(m_Queue.front());
				m_Queue.pop();
			}
		}

		// Hard eviction
		if ((m_CurrentSizeInBytes + logSize) > m_MaxSizeInBytes * m_EvictionThresholdHard) {
			size_t targetSize = static_cast<size_t>(m_MaxSizeInBytes * m_EvictionStopThresholdHard);
			while (m_CurrentSizeInBytes > targetSize && !m_Queue.empty()) {
				m_CurrentSizeInBytes -= computeLogSizeInBytes(m_Queue.front());
				m_Queue.pop();
			}
		}

		m_Queue.push(log);
		m_CurrentSizeInBytes += logSize;
		return true;
	}
};