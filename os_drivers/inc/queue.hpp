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
	bool put(T& data) {
		BaseType_t xHigherPriorityTaskWoken = pdFALSE, xResult;

		xResult = xQueueSendFromISR(handle, &data, &xHigherPriorityTaskWoken );

		if ( xResult != pdFAIL ){
			portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
			return true;
		} else {
			return false;
		}
	}
	T front() {
		T data;
		xQueuePeekFromISR(handle, &data);
		return data;
	}
	T take(bool& error) {
		T data;
		BaseType_t xResult = xQueueReceive(handle, &data, portMAX_DELAY);
		error = !xResult;
		return data;
	}
};

#endif // QUEUE_HPP
