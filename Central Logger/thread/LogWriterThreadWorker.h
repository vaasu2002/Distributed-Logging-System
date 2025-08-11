#pragma once
#include "BaseThreadWorker.h"
#include "LogPriorityQueue.h"
#include "LogDLQueue.h"
#include "SinkFactory.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <memory>
#define NOMINMAX

/**
* @class LogWriterThread
* @brief Background worker thread responsible for draining log queues and writing logs to configurable output sinks.
*
* @details
* - This worker continuously reads logs from:
*	1. Priority Queue [Primary]
*		- Holds logs ordered by importance or timestamp.
*		- Processed in batches for efficiency.
*	2. Dead Letter Queue [Secondary]
*		- Stores logs that could not be processed in the primary flow (e.g., due to overload).
*		- Processed only when the priority queue is not overloaded.
*
* - Logs delayed beyond a configured threshold are tagged as `[BACKLOG]` to indicate a processing delay.
* - Dead-letter logs are tagged as `[BACKLOG.DLQ]`.
* - Supports configurable output sinks (console, file, multiple destinations, etc.)
*
*/
class LogWriterThread : public BaseThreadWorker {
	LogPriorityQueue* m_PriorityQueue;   // Pointer to the main priority log queue.
	LogDLQueue* m_DeadLetterQueue;       // Pointer to the fallback dead-letter log queue.
	std::chrono::system_clock::time_point m_Latest; // Timestamp of the latest processed log.
	std::unique_ptr<ISink> m_Sink;    // Output sink for writing logs.

public:
	/**
	* @brief Constructs a LogWriterThread.
	* @param priorityQueue Pointer to the primary priority queue.
	* @param deadLetterQueue Pointer to the dead-letter queue.
	* @param sink Unique pointer to the output sink (defaults to console if null).
	*/
	LogWriterThread(LogPriorityQueue* priorityQueue, LogDLQueue* deadLetterQueue,
		std::unique_ptr<ISink> sink = nullptr)
		: m_PriorityQueue(priorityQueue), m_DeadLetterQueue(deadLetterQueue),
		m_Latest((std::chrono::system_clock::time_point::min)()),
		m_Sink(sink ? std::move(sink) : SinkFactory::createFileSink("all_logs.log"))
	{
		m_ThreadWorkerName = "LogWriterThread";
	}

	/**
	* @brief Set a new sink for log output
	* @param sink Unique pointer to the new sink
	*/
	void setSink(std::unique_ptr<ISink> sink) {
		if (sink) {
			m_Sink = std::move(sink);
		}
	}

	/**
	* @brief Get the current sink (for checking status, etc.)
	* @return Pointer to the current sink
	*/
	ISink* getSink() const {
		return m_Sink.get();
	}

protected:
	/**
	* @brief Main worker loop that reads from queues and writes logs.
	*
	* @details
	* - Processing flow:
	*	- Dequeue up to `BATCH_SIZE` logs from the priority queue.
	*	- Mark logs as `[BACKLOG]` if their timestamp is older than `BACKLOG_THRESHOLD` relative to the latest processed log.
	*	- Periodically pull one log from the dead-letter queue if the priority queue is not overloaded.
	*	- Write all logs to the configured sink.
	*/
	void run() override {
		const size_t BATCH_SIZE = 20;  // Number of logs to process in a single batch.
		const std::chrono::milliseconds BACKLOG_THRESHOLD(200); // Delay threshold for backlog tagging.
		std::vector<Log> batch;

		while (!isStopRequested()) {
			// Skip processing if sink is not ready
			if (!m_Sink || !m_Sink->isReady()) {
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				continue;
			}

			batch.clear();

			// Drain logs from the priority queue
			for (size_t i = 0; i < BATCH_SIZE; ++i) {
				Log log;
				if (!m_PriorityQueue->dequeue(log)) {
					// Queue empty or stop requested, process what we have so far.
					break;
				}
				batch.push_back(log);
			}

			// Process each log in the batch
			for (const Log& log : batch) {
				auto logTime = log.getTimestamp();
				auto delay = std::chrono::duration_cast<std::chrono::milliseconds>(m_Latest - logTime);
				std::string finalLog = log.toString();

				if (delay > BACKLOG_THRESHOLD) {
					finalLog = "[BACKLOG]" + finalLog;
				}

				if (m_Latest < logTime) {
					m_Latest = logTime; // Track the latest timestamp seen.
				}

				// Write to sink instead of directly to console
				m_Sink->write(finalLog);
			}

			// If main queue is healthy(not overloaded), process from dead-letter queue
			if (!m_PriorityQueue->isOverloaded() && m_DeadLetterQueue->size() > 0) {
				Log deadLog;
				if (m_DeadLetterQueue->dequeue(deadLog)) {
					std::string finalDeadLog = "[BACKLOG.DLQ]" + deadLog.toString();
					m_Sink->write(finalDeadLog);
				}
			}

			// Flush the sink periodically for better performance
			if (!batch.empty()) {
				m_Sink->flush();
			}
		}

		// Final flush when stopping
		if (m_Sink) {
			m_Sink->flush();
		}
	}
};