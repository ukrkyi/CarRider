/* (c) 2020 ukrkyi */
#include <stm32f4xx_hal.h>

#include "motor_dc.h"
#include "pwm.h"
#include "ultrasonic.h"
#include "led.h"

#include "system.h"

static inline void delay(unsigned ms) {
	volatile unsigned i = 0;
	unsigned freq = SystemCoreClock/1000; // kHz
	unsigned limit = ms*freq/12;
	for (; i < limit; i++) __NOP();
}

void blink(LED& led, int n, int wait_till = 0) {
	const unsigned t = 250;
	for (int i = 0; i < n; i++) {
		led.on();
		delay(t);
		led.off();
		delay(t);
	}
	if (wait_till > n)
		delay(t * 2 * (wait_till - n));
}


extern "C" void TIM2_IRQHandler(void) {
	Ultrasonic::getInstance().processEcho(0);
}

int main()
{
	SystemConfig();

	LED &led = LED::getInstance();

	led.on();
	delay(2000);
	led.off();

	MotorDC & drive = MotorDC::getInstance(MOTOR_DRIVE),
				&turn = MotorDC::getInstance(MOTOR_TURN);

	Ultrasonic & range = Ultrasonic::getInstance();

	NVIC_EnableIRQ(TIM2_IRQn);

	range.start();

	const float refDistance = 100; // 0.1m
	static float distance;

	while(1) {
		led.off();
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
		delay(100);
	}
}
