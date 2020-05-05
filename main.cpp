/* (c) 2020 ukrkyi */
#include <stm32f4xx_hal.h>

#include "motor_dc.h"
#include "pwm.h"
#include "ultrasonic.h"
#include "led.h"

#include "system.h"
#include "eventgroup.h"

#include "uart.h"

#include "FreeRTOS.h"
#include "task.h"


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

int main()
{
	SystemConfig();

	xTaskCreateStatic(mainTask, "main", STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, xStack[0], xTaskBuffer);

	/* Start the scheduler. */
	vTaskStartScheduler();

	/* If all is well, the scheduler will now be running, and the following line
	will never be reached.  If the following line does execute, then there was
	insufficient FreeRTOS heap memory available for the idle and/or timer tasks
	to be created.  See the memory management section on the FreeRTOS web site
	for more details. */
	for( ;; );
}
