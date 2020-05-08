/* (c) 2020 ukrkyi */
#ifndef QUEUE_HPP
#define QUEUE_HPP

#include <FreeRTOS.h>
#include <queue.h>

template <typename T> class Queue
{
	StaticQueue_t storage;
	QueueHandle_t handle;
public:
	Queue(uint8_t * buffer, size_t size) {
		handle = xQueueCreateStatic(size, sizeof (T), buffer, &storage);
	}
	Queue() = delete;
	Queue(const Queue&) = delete;
	bool putISR(T& data) {
		BaseType_t xHigherPriorityTaskWoken = pdFALSE, xResult;

		xResult = xQueueSendFromISR(handle, &data, &xHigherPriorityTaskWoken );

		if ( xResult != pdFAIL ){
			portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
			return true;
		} else {
			return false;
		}
	}
	bool put(T& data) {
		if (xPortIsInsideInterrupt())
			return putISR(data);
		else {
			return xQueueSend(handle, &data, portMAX_DELAY) == pdTRUE;
		}
	}
	T front() {
		T data;
		xQueuePeekFromISR(handle, &data);
		return data;
	}
	bool empty() {
		return uxQueueMessagesWaiting(handle) == 0;
	}
	inline T take(bool& valid, bool block = true) {
		T data;
		BaseType_t xResult = xQueueReceive(handle, &data, block ? portMAX_DELAY : 0);
		valid = xResult;
		return data;
	}
};

#endif // QUEUE_HPP
