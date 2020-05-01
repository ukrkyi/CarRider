/* (c) 2020 ukrkyi */
#include <stm32f4xx_hal.h>

#include "motor_dc.h"
#include "pwm.h"
#include "ultrasonic.h"
#include "led.h"

#include "system.h"
#include "eventgroup.h"

#include "FreeRTOS.h"
#include "task.h"

extern "C" void TIM2_IRQHandler(void) {
	Ultrasonic::getInstance().processEcho(0);
}

#define STACK_SIZE	configMINIMAL_STACK_SIZE
#define TASK_NUM	1

StaticTask_t xTaskBuffer[TASK_NUM];

StackType_t xStack[TASK_NUM][ STACK_SIZE ];

void mainTask(void * parameters) {
	LED& led = LED::getInstance();

	MotorDC & drive = MotorDC::getInstance(MOTOR_DRIVE),
				&turn = MotorDC::getInstance(MOTOR_TURN);

	Ultrasonic & range = Ultrasonic::getInstance();

	NVIC_EnableIRQ(TIM2_IRQn);
	NVIC_SetPriority(TIM2_IRQn, 5);

	range.start();

	EventGroup &sensors = EventGroup::getInstance();

	const float refDistance = 100; // 0.1m
	static float distance;

	while(1) {
		led.off();
		sensors.wait(ULTRASONIC_MEASUREMENT_COMPLETED);
		distance = range.getDistance();
		if (distance - refDistance > 100) {
			drive.run((distance - refDistance >= 500) ? 50 : (int) (distance - refDistance) / 10);
		} else if (distance > 1) {
			if (distance - refDistance < -50)
				drive.run(-30);
			else
				drive.stop();
		} else {
			led.on(); // error
			drive.stop();
		}
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
