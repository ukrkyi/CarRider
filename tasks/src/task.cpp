/* (c) 2020 ukrkyi */
#include "task.hpp"

Task::Task(const char * name, Task * task, UBaseType_t priority)
{
	handle = xTaskCreateStatic(start, name, STACK_SIZE, task, priority, xStack, &xTaskBuffer);
}

void Task::notify()
{
	xTaskNotify(handle, 0, eNoAction);
}

void Task::notifyISR()
{
	BaseType_t xHigherPriorityTaskWoken;

	xHigherPriorityTaskWoken = pdFALSE;

	xTaskNotifyFromISR(handle, 0, eNoAction,  &xHigherPriorityTaskWoken);

	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

void Task::start(void *task)
{
	((Task *) task)->run();
}
