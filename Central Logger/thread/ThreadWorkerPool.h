#pragma once
#include <vector>
#include <memory>
#include <iostream>
#include "MQReaderThreadWorker.h"
#include "LogDLQueue.h"
#include "LogPriorityQueue.h" 
/*
* @class ThreadWorkerPool
* @brief Manages a pool of BaseThreadWorker threads.
*/
class ThreadWorkerPool {
	// Collection of Thread worker
	std::vector<std::unique_ptr<MQReaderThreadWorker>> m_Workers;
	bool m_Running = false;
public:
	ThreadWorkerPool() = default;
	ThreadWorkerPool(int numWorkers,
		LogPriorityQueue* priorityQueue,
		LogDLQueue* deadLetterQueue){
		for (int i = 0; i < numWorkers; ++i) {
			m_Workers.push_back(std::make_unique<MQReaderThreadWorker>(i,priorityQueue, deadLetterQueue));
		}
	}
	~ThreadWorkerPool() {
		stopAll();
	}

	/*
	* @brief Adds a new worker to the pool.
	*/
	void addWorker(std::unique_ptr<MQReaderThreadWorker> worker) {
		m_Workers.push_back(std::move(worker));
	}

	/*
	* @brief Starts all workers in the pool.
	*/
	void startAll() {
		if (m_Running) return; // Prevents double starting 
		m_Running = true;
		for (auto& worker : m_Workers) {
			worker->start();
		}
	}

	/**
	* @brief Stops all workers in the pool.
	*/
	void stopAll() noexcept {
		for (auto& worker : m_Workers) {
			worker->stop();
		}
	}
};