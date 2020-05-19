/* (c) 2020 ukrkyi */
#ifndef TASK_HPP
#define TASK_HPP

#include <FreeRTOS.h>
#include <task.h>

#define STACK_SIZE	configMINIMAL_STACK_SIZE * 2

#define WIFI_TASK_PRIORITY	tskIDLE_PRIORITY + 1
#define LOG_TASK_PRIORITY	tskIDLE_PRIORITY + 2
#define MAIN_TASK_PRIORITY	tskIDLE_PRIORITY + 3
#define POSITION_TASK_PRIORITY	tskIDLE_PRIORITY + 4

class Task
{
	TaskHandle_t handle;
	StaticTask_t xTaskBuffer;
	StackType_t xStack[ STACK_SIZE ];
	static void start(void * task);
protected:
	Task(const char * name, Task * task, UBaseType_t priority);
	virtual void run(void) = 0;
	uint32_t wait(uint32_t clear = 0);
public:
	Task() = delete;
	Task(const Task &) = delete;
	void notify(uint32_t bit = 0);
	void notifyISR(uint32_t bit = 0);
};

#endif // TASK_HPP
