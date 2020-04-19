/* (c) 2020 ukrkyi */
#include <stm32f4xx_hal.h>

#include "motor_dc.h"
#include "ultrasonic.h"
#include "led.h"

static inline void delay(unsigned ms) {
	volatile unsigned i = 0;
	unsigned freq = SystemCoreClock/1000; // kHz
	unsigned limit = ms*freq/12;
	for (; i < limit; i++) __NOP();
}

void blink(LED& led, int n) {
	for (int i = 0; i < n; i++) {
		led.on();
		delay(250);
		led.off();
		delay(250);
	}
}

LED led;
Ultrasonic * range = NULL;
volatile float distance = 0;

extern "C" void TIM2_IRQHandler(void) {
	if (range != NULL)
		distance = range->processEcho(0);
}

int main()
{

	led.on();
	delay(2000);
	led.off();

	MotorDC drive(GPIO_PIN_0, GPIO_PIN_1), rotate(GPIO_PIN_2, GPIO_PIN_3);
	Ultrasonic sensor(GPIOB, GPIO_PIN_0, GPIOA, GPIO_PIN_15,
			  TIM3, TIM_CHANNEL_3, TIM2, TIM_CHANNEL_1, 100000);

	NVIC_EnableIRQ(TIM2_IRQn);
	range = &sensor;

	sensor.start();

	const float refDistance = 200; // 0.1m

	while(1) {
		led.off();
		if (distance - refDistance > 250)
			drive.run(FORWARD);
		else if (distance > 1) {
			if (distance - refDistance < -100)
				drive.run(BACKWARD);
			else
				drive.stop();
		} else {
			led.on(); // error
			drive.stop();
		}
		delay(100);
	}
}
