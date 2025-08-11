#pragma once

#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include "Log.h"
#include "BaseQueue.h"

/**
* @class LogTimestampCompare
* @brief Comparator giving high priority to earlier timestamps
*/
class LogTimestampCompare {
public:
	bool operator()(const Log& a, const Log& b) const {
		return a.getTimePoint() > b.getTimePoint();
	}
};

/**
* @class LogPriorityQueue
* @brief Thread-safe priority queue that tracks memory usage.
*/
class LogPriorityQueue : public IBaseQueue {
private:
	// Real queue storing logs with high priority to earlier timestamp.
	std::priority_queue<Log, std::vector<Log>, LogTimestampCompare> m_Queue;

	// Variables for thread-safe operations.
	mutable std::mutex m_Mutex;
	std::condition_variable m_CV;
	std::atomic<bool> m_StopFlag{ false };

	// Variables for tracking size.
	size_t m_CurrentSizeInBytes = 0;
	size_t m_MaxCapacityInBytes;

public:
	explicit LogPriorityQueue(size_t maxBytesCapacity = 15 * 1024 * 1024) // Default 15MB cap
		: m_MaxCapacityInBytes(maxBytesCapacity) {}

	bool enqueue(const Log& log) override {
		std::lock_guard<std::mutex> lock(m_Mutex);
		bool res  = enqueueInternal(log);
		if (res) {
			m_CV.notify_one();
		}
		return res;
	}

	bool enqueueBatch(const std::vector<Log>& logs) override {
		std::lock_guard<std::mutex> lock(m_Mutex);
		for (const auto& log : logs) {
			if (!enqueueInternal(log)) {
				continue;
			}
		}
		m_CV.notify_all();
		return true;
	}

	bool enqueueInternal(const Log& log) override {
		const size_t logSizeInBytes = computeLogSizeInBytes(log);

		// Reject if this log exceeds capacity
		if (logSizeInBytes > m_MaxCapacityInBytes) {
			// TODO: Added in large_log.txt file
			return false;
		}

		// Reject if adding would exceed capacity
		if (m_CurrentSizeInBytes + logSizeInBytes > m_MaxCapacityInBytes) {
			// TODO: Retry mechanism is trigger added again in Message Queue (LEVEL >= ERROR)
			return false;
		}

		m_Queue.push(log);
		m_CurrentSizeInBytes += logSizeInBytes;
		return true;
	}

	bool dequeue(Log& log) override {
		std::unique_lock<std::mutex> lock(m_Mutex);
		m_CV.wait(lock, [this]() {
			return !m_Queue.empty() || m_StopFlag.load();
		});

		if (m_Queue.empty()) return false;

		log = m_Queue.top();
		m_Queue.pop();
		m_CurrentSizeInBytes -= computeLogSizeInBytes(log);
		return true;
	}

	size_t dequeueBatch(std::vector<Log>& outLogs, size_t maxCount) override {
		std::unique_lock<std::mutex> lock(m_Mutex);
		m_CV.wait(lock, [this]() {
			return !m_Queue.empty() || m_StopFlag.load();
		});

		size_t count = 0;
		while (count < maxCount && !m_Queue.empty()) {
			Log log = m_Queue.top();
			m_Queue.pop();
			m_CurrentSizeInBytes -= computeLogSizeInBytes(log);
			outLogs.push_back(std::move(log));
			++count;
		}
		return count;
	}

	void shutdown() {
		m_StopFlag = true;
		m_CV.notify_all();
	}

	size_t size() const override {
		std::lock_guard<std::mutex> lock(m_Mutex);
		return m_Queue.size();
	}

	size_t capacityBytes() const {
		return m_MaxCapacityInBytes;
	}

	size_t usedBytes() const {
		return m_CurrentSizeInBytes;
	}

	double currentMemoryUsageMB() const {
		return m_CurrentSizeInBytes / (1024.0 * 1024.0);
	}

	bool isOverloaded(double threshold = 0.6) const {
		return (static_cast<double>(usedBytes()) / capacityBytes()) > threshold;
	}

	bool empty() const {
		std::lock_guard<std::mutex> lock(m_Mutex);
		return m_Queue.empty();
	}

	void reset() override {
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_CurrentSizeInBytes = 0;
		while (!m_Queue.empty()) {
			m_Queue.pop();
		}
	}
};