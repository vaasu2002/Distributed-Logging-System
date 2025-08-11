#pragma once
#include "BaseThreadWorker.h"
#include "LogPriorityQueue.h"
#include "LogDLQueue.h"
#include "Log.h"
#include <windows.h>
#include <mq.h>
#include <vector>
#include <iostream>

/*
* @class MQReaderWorker
* @brief Handles the logic of consuming logs from Message Queue
*/
class MQReaderThreadWorker : public BaseThreadWorker {
	int m_ThreadId;
	LogPriorityQueue* m_LogPriorityQueue;
	LogDLQueue* m_LogDLQueue;

	/*
	* @details
	* - Triggered when shutdown is taking place
	* - Adds all the left over logs to appropriate queue
	*/
	void flushLeftovers(std::vector<Log>& localBuffer) {
		std::cout << "[" << m_ThreadWorkerName << "] Flushing leftovers to main queue.\n";
		for (const auto& log : localBuffer) {
			m_LogPriorityQueue->enqueue(log);
		}
	}

	/*
	* @brief Converts wide string to UTF-8 string safely
	*/
	std::string wideToUtf8(const std::wstring& wideStr) {
		if (wideStr.empty()) return std::string();

		int size = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, nullptr, 0, nullptr, nullptr);
		if (size <= 0) return std::string();

		std::string result(size - 1, '\0');
		WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, &result[0], size, nullptr, nullptr);
		return result;
	}

public:
	MQReaderThreadWorker(int id, LogPriorityQueue* pq, LogDLQueue* dlq)
		: BaseThreadWorker("ReaderThread-" + std::to_string(id)),
		m_ThreadId(id), m_LogPriorityQueue(pq), m_LogDLQueue(dlq) {}

protected:
	void run() override {
		// Use the original shared queue name
		const WCHAR* queueFormatName = L"DIRECT=OS:.\\private$\\logqueue";
		QUEUEHANDLE queueHandle = nullptr;

		HRESULT hr = MQOpenQueue(queueFormatName, MQ_RECEIVE_ACCESS, MQ_DENY_NONE, &queueHandle);
		if (FAILED(hr)) {
			std::cerr << "[" << m_ThreadWorkerName << "] Failed to open MSMQ. HR=0x"
				<< std::hex << hr << std::dec << "\n";
			return;
		}

		std::vector<Log> localBuffer;
		const size_t BATCH_SIZE = 50;
		const int MAX_BUFFER_SIZE = 2048; // bytes

		// Pre-allocate receive buffer and property structures once (reuse each loop)
		std::vector<BYTE> msgBuffer(MAX_BUFFER_SIZE);
		MSGPROPID aMsgPropId[1] = { PROPID_M_BODY };
		MQPROPVARIANT aMsgPropVar[1] = {};
		MQMSGPROPS msgProps = {};

		aMsgPropVar[0].vt = VT_UI1 | VT_VECTOR;
		aMsgPropVar[0].caub.pElems = msgBuffer.data();
		aMsgPropVar[0].caub.cElems = MAX_BUFFER_SIZE;

		msgProps.cProp = 1;
		msgProps.aPropID = aMsgPropId;
		msgProps.aPropVar = aMsgPropVar;
		msgProps.aStatus = nullptr;

		std::cout << "[" << m_ThreadWorkerName << "] Started, using shared queue\n";

		while (!isStopRequested()) {
			// Reset buffer size before each receive
			aMsgPropVar[0].caub.cElems = MAX_BUFFER_SIZE;

			hr = MQReceiveMessage(queueHandle, 1000, MQ_ACTION_RECEIVE, &msgProps, nullptr, nullptr, nullptr, nullptr);

			if (SUCCEEDED(hr)) {
				DWORD bytesReceived = aMsgPropVar[0].caub.cElems;

				if (bytesReceived == 0) {
					continue; // Skip empty messages
				}

				std::string message;

				// Check if data looks like wide string
				if (bytesReceived >= sizeof(WCHAR) && (bytesReceived % sizeof(WCHAR) == 0)) {
					size_t wcharCount = bytesReceived / sizeof(WCHAR);
					std::wstring wideMessage(reinterpret_cast<WCHAR*>(msgBuffer.data()), wcharCount);

					// Remove trailing null/space characters
					while (!wideMessage.empty() && (wideMessage.back() == L'\0' || wideMessage.back() == L' ')) {
						wideMessage.pop_back();
					}

					message = wideToUtf8(wideMessage);
				}
				else {
					// Treat as ASCII/UTF-8 message
					message = std::string(reinterpret_cast<char*>(msgBuffer.data()), bytesReceived);

					// Remove trailing null characters
					while (!message.empty() && message.back() == '\0') {
						message.pop_back();
					}
				}

				if (message.empty()) {
					continue; // Skip empty messages after conversion
				}

					Log logEntry = Log::parseFromString(message);

					// Dead-lettering policy: send low-priority logs to DLQ when main queue is full
					if (logEntry.getLogLevel() <= LogLevel::INFO && m_LogPriorityQueue->size() > 1000) {
						m_LogDLQueue->enqueue(logEntry);
					}
					else {
						localBuffer.push_back(logEntry);
					}

					// Batch enqueue to reduce lock contention
					if (localBuffer.size() >= BATCH_SIZE) {
						for (const auto& log : localBuffer) {
							m_LogPriorityQueue->enqueue(log);
						}
						localBuffer.clear();
					}
			}
			else if (hr == MQ_ERROR_IO_TIMEOUT) {
				// Timeout is normal, continue polling
				continue;
			}
			else {
				std::cerr << "[" << m_ThreadWorkerName << "] MSMQ error: 0x"
					<< std::hex << hr << std::dec << "\n";

				// Handle specific error cases
				if (hr == MQ_ERROR_QUEUE_NOT_FOUND) {
					std::cerr << "[" << m_ThreadWorkerName << "] Queue not found. Exiting thread.\n";
					break;
				}
				else if (hr == MQ_ERROR_ACCESS_DENIED) {
					std::cerr << "[" << m_ThreadWorkerName << "] Access denied. Check permissions.\n";
					break;
				}
			}
		}

		if (queueHandle) {
			MQCloseQueue(queueHandle);
		}
		flushLeftovers(localBuffer);
		std::cout << "[" << m_ThreadWorkerName << "] Exited.\n";
	}
};