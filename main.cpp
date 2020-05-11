/* (c) 2020 ukrkyi */
#include <stm32f4xx_hal.h>

#include <stm32f4xx_ll_system.h>

#include "motor_dc.h"
#include "pwm.h"
#include "ultrasonic.h"
#include "led.h"

#include "system.h"
#include "eventgroup.h"

#include "FreeRTOS.h"
#include "task.h"

#include "buffer.h"

#include "wifi.h"
#include "position.h"

#include <stdio.h>

#define STACK_SIZE	configMINIMAL_STACK_SIZE * 2
#define TASK_NUM	1

StaticTask_t xTaskBuffer[TASK_NUM];

StackType_t xStack[TASK_NUM][ STACK_SIZE ];

extern "C" void mainTask(void * parameters) {
//	LED& led = LED::getInstance();

//	MotorDC & drive = MotorDC::getInstance(MOTOR_DRIVE),
//				&turn = MotorDC::getInstance(MOTOR_TURN);
	const WiFi::Socket comp = {"192.168.1.98", 1488};
	const WiFi::AccessPoint ap = {"KSE_2.4", "quang1967", true};
	WiFi::Data data;

	LED & led = LED::getInstance();
	WiFi & wifi = WiFi::getInstance();

	EventGroup &evt = EventGroup::getInstance();

	evt.clear(WIFI_COMMAND_ERROR);

	Event res;

	wifi.sendCommand(WiFi::POWER_ON);

	res = evt.wait(WIFI_COMMAND_ERROR | WIFI_STATE_CHANGED);

	if (res & WIFI_COMMAND_ERROR)
		while (1); // TODO handle error

	vTaskDelay(1000);

	if (wifi.getState() != WiFi::WIFI_CONNECTED) {
		wifi.sendCommand(WiFi::WIFI_CONNECT, (void *) &ap);

		res = evt.wait(WIFI_COMMAND_ERROR | WIFI_STATE_CHANGED);

		if (res & WIFI_COMMAND_ERROR)
			while (1); // TODO handle error
	}

	do {
		wifi.sendCommand(WiFi::TCP_CONNECT, (void *) &comp);
		res = evt.wait(WIFI_COMMAND_ERROR | WIFI_STATE_CHANGED);

		if (res & WIFI_COMMAND_ERROR) {
			led.on();
			vTaskDelay(1000);
			continue;
		}
	} while (wifi.getState() != WiFi::TCP_CONNECTED);

	led.off();

	wifi.sendCommand(WiFi::TCP_SEND, &(data = {3, "o/\n"}));
	evt.wait(WIFI_COMMAND_ERROR | WIFI_CMD_PROCESSED);

	Ultrasonic & range = Ultrasonic::getInstance();

	range.start();

	static float distance;
	static char str[25];
	static unsigned size;

	while(1) {
		evt.wait(ULTRASONIC_NEW_DATA);

		distance = range.getDistance();
		size = snprintf(str, 25, "Ultrasonic: %d.%d mm\n", (int) distance, (int) (distance * 10) % 10);
		data = {size, str};

		evt.clear(WIFI_CMD_PROCESSED);
		wifi.sendCommand(WiFi::TCP_SEND, &data);
		evt.wait(WIFI_CMD_PROCESSED);
		vTaskDelay(5000);
	}
}

int main()
{
	SystemConfig();

#ifndef NDEBUG
	LL_DBGMCU_EnableDBGSleepMode();
#endif

	xTaskCreateStatic(mainTask, "main", STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, xStack[0], xTaskBuffer);

	WiFi::getInstance(); // create WiFi task
	Position::getInstance(); // create Position task

	/* Start the scheduler. */
	vTaskStartScheduler();

	/* If all is well, the scheduler will now be running, and the following line
	will never be reached.  If the following line does execute, then there was
	insufficient FreeRTOS heap memory available for the idle and/or timer tasks
	to be created.  See the memory management section on the FreeRTOS web site
	for more details. */
	for( ;; );
}
