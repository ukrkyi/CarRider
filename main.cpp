/* (c) 2020 ukrkyi */
#include <stm32f4xx_hal.h>

#include "motor_dc.h"
#include "pwm.h"
#include "led.h"

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

int main()
{
	LED led;

	led.on();
	delay(2000);
	led.off();

	PWM run_forward(GPIOA, GPIO_PIN_0, TIM5, TIM_CHANNEL_1, 160000),
			run_backward(GPIOA, GPIO_PIN_1, TIM_CHANNEL_2, run_forward),
			turn_right(GPIOA, GPIO_PIN_2, TIM_CHANNEL_3, run_forward),
			turn_left(GPIOA, GPIO_PIN_3, TIM_CHANNEL_4, run_forward);

	MotorDC drive(run_forward, run_backward), turn(turn_right, turn_left);

	drive.run(50);
	delay(1000);
	drive.run(-50);
	delay(1000);
	drive.stop();

	led.on();


	while(1) {}
}
