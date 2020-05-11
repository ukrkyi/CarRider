/* (c) 2020 ukrkyi */
#include "task.hpp"

Task::Task(const char * name, Task * task, UBaseType_t priority)
{
	handle = xTaskCreateStatic(start, name, STACK_SIZE, task, priority, xStack, &xTaskBuffer);
}


uint32_t Task::wait(uint32_t clear)
{
	uint32_t result;
	if (!clear) {
		xTaskNotifyWait(0x0, 0x0, &result, portMAX_DELAY);
		return result;
	}
	xTaskNotifyWait(0x0, clear, &result, 0);
	while (!(result & clear)) {
		xTaskNotifyWait(0x0, clear, &result, portMAX_DELAY);
	}
	return result;
}

void Task::notify(uint32_t bit)
{
	if (!bit)
		xTaskNotify(handle, 0, eNoAction);
	else
		xTaskNotify(handle, bit, eSetBits);
}

void Task::notifyISR(uint32_t bit)
{
	BaseType_t xHigherPriorityTaskWoken;

	xHigherPriorityTaskWoken = pdFALSE;

	if (!bit)
		xTaskNotifyFromISR(handle, 0, eNoAction,  &xHigherPriorityTaskWoken);
	else
		xTaskNotifyFromISR(handle, bit, eSetBits,  &xHigherPriorityTaskWoken);

	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

void Task::start(void *task)
{
	((Task *) task)->run();
}
