#pragma once
#ifndef BASE_QUEUE_H
#define BASE_QUEUE_H

#include "Log.h"
#include <vector>

class IBaseQueue {
protected:
	size_t computeLogSizeInBytes(const Log& log) const {
		return sizeof(log) +
			log.getMessage().capacity() +
			log.getSource().capacity();
	}
public:
	virtual bool enqueue(const Log& log) = 0;
	virtual bool dequeue(Log& log) = 0;
	virtual bool enqueueBatch(const std::vector<Log>& logs) = 0;
	virtual size_t dequeueBatch(std::vector<Log>& outLogs, size_t maxCount) = 0;
	virtual size_t size() const = 0;
	virtual void reset() = 0;
	virtual bool enqueueInternal(const Log& log) { return false; }

	virtual ~IBaseQueue() = default;
};

#endif // !BASE_QUEUE_H