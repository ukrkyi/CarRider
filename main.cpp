/* (c) 2020 ukrkyi */
#include <stm32f4xx_hal.h>

#include <stm32f4xx_ll_system.h>

#include "motor_dc.h"
#include "pwm.h"
#include "ultrasonic.h"
#include "led.h"

#include "system.h"
#include "eventgroup.h"

#include "uart.h"

#include "FreeRTOS.h"
#include "task.h"

#include "buffer.h"

#include <stdio.h>

#define STACK_SIZE	configMINIMAL_STACK_SIZE * 2
#define TASK_NUM	2

StaticTask_t xTaskBuffer[TASK_NUM];

StackType_t xStack[TASK_NUM][ STACK_SIZE ];

void mainTask(void * parameters) {
//	LED& led = LED::getInstance();

//	MotorDC & drive = MotorDC::getInstance(MOTOR_DRIVE),
//				&turn = MotorDC::getInstance(MOTOR_TURN);

	Ultrasonic & range = Ultrasonic::getInstance();


	range.start();

	EventGroup &sensors = EventGroup::getInstance();
	UART &uart = UART::getInstance();

	static float distance;
	static char buf[10];
	int size;

	while(1) {
		sensors.wait(ULTRASONIC_MEASUREMENT_COMPLETED);
		distance = range.getDistance();
		size = snprintf(buf, 10, "%d.%d ", (int) distance, (int) (distance * 10) % 10);
		uart.send((uint8_t *) buf, size > 10 ? 10 : size);
		uart.send((const uint8_t *) "mm\n", 3);
		vTaskDelay(500);
	}
}

void wifiTask(void * parameters) {
	static uint8_t queue_buffer[sizeof (size_t) * 5];
	static Buffer<10> rxBuffer;

	LED& led = LED::getInstance();
	UART &uart = UART::getInstance();

	Queue<size_t> q(queue_buffer, 5);

	bool err = false;

	size_t pos;

	uart.startRx(rxBuffer, rxBuffer.length(), q);

	while(1) {
		pos = q.take(err);
		if (err)
			while(1);

		Buffer<10>::chunk data = rxBuffer.newData(pos, &err);
		while (data.length() != 0) {
			if (data.beginsWith("+")) {
				led.on();
			} else if (data.beginsWith("-")){
				led.off();
			} else if (data.beginsWith("123")) {
				uart.send((const uint8_t *)"123\n", 4);
			}
			rxBuffer.next(data, 1);
		}
	}
}

int main()
{
	SystemConfig();

#ifndef NDEBUG
	LL_DBGMCU_EnableDBGSleepMode();
#endif

	xTaskCreateStatic(mainTask, "main", STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, xStack[0], xTaskBuffer);
	xTaskCreateStatic(wifiTask, "wifi", STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, xStack[1], xTaskBuffer + 1);

	/* Start the scheduler. */
	vTaskStartScheduler();

	/* If all is well, the scheduler will now be running, and the following line
	will never be reached.  If the following line does execute, then there was
	insufficient FreeRTOS heap memory available for the idle and/or timer tasks
	to be created.  See the memory management section on the FreeRTOS web site
	for more details. */
	for( ;; );
}
