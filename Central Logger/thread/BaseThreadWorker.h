#pragma once
#ifndef BASE_THREAD_WORKER_H
#define BASE_THREAD_WORKER_H

#include <atomic>
#include <thread>
#include <string>


/**
* @class BaseThreadWorker
* @brief Abstract base class for managing thread-based worker tasks.
*
* @details
* - This class provides a thread management interface that can be extended.
* - The thread is guaranteed to be joined upon the object's destruction, even if stop() is not used explicitly by subclass.
*/
class BaseThreadWorker {
	void runWrapper() {
		run();
	}
protected:
	std::string m_ThreadWorkerName;
	// Atomic flag to signal a request for termination of worker thread.
	std::atomic<bool> m_Stop{ false };
	std::thread m_Thread;

	/**
	* @brief The main function for the worker thread.
	*
	* @details 
	* - This is a pure virtual function that must be implemented by the derived class.
	* - The implementation should contain the logic to be executed in the separate thread.
	* - IMPORANT: It should periodically check `isStopRequested()` to ensure it can be stopped gracefully.
	*/
	virtual void run() = 0;

	/**
	* @brief Check if the worker thread is requested to stop.
	* @return true if stop is requested, false otherwise.
	*/
	bool isStopRequested() const noexcept {
		return m_Stop.load();
	}
public:
	// Constructors
	BaseThreadWorker() = default;
	BaseThreadWorker(const std::string& threadWorkerName) :m_ThreadWorkerName(threadWorkerName) {}
	// Destrcutors
	virtual ~BaseThreadWorker() {
		stop();
	}

	/**
	* @brief Start the worker thread.
	*/
	void start() {
		m_Stop = false;
		m_Thread = std::thread(&BaseThreadWorker::runWrapper, this);
	}

	/**
	* @brief Stop the worker thread.
	*/
	void stop() {
		m_Stop = true;
		if (m_Thread.joinable())
			m_Thread.join();
	}
};

#endif // !BASE_THREAD_WORKER_H