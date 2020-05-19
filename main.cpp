/* (c) 2020 ukrkyi */
#include <stm32f4xx_hal.h>

#include <stm32f4xx_ll_system.h>

#include "system.h"

#include "FreeRTOS.h"
#include "task.h"

#include "wifi.h"
#include "position.h"
#include "maintask.h"
#include "log.h"

#include <stdio.h>

int main()
{
	SystemConfig();

#ifndef NDEBUG
	LL_DBGMCU_EnableDBGSleepMode();
#endif

	Main::getInstance();
	WiFi::getInstance(); // create WiFi task
	Position::getInstance(); // create Position task
	Log::getInstance();

	/* Start the scheduler. */
	vTaskStartScheduler();

	/* If all is well, the scheduler will now be running, and the following line
	will never be reached.  If the following line does execute, then there was
	insufficient FreeRTOS heap memory available for the idle and/or timer tasks
	to be created.  See the memory management section on the FreeRTOS web site
	for more details. */
	for( ;; );
}
