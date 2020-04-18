/* (c) 2020 ukrkyi */
#include <stm32f4xx_hal.h>

#include "motor_dc.h"
#include "led.h"

static inline void delay(unsigned ms) {
	volatile unsigned i = 0;
	unsigned freq = SystemCoreClock/1000; // kHz
	unsigned limit = ms*freq/12;
	for (; i < limit; i++) __NOP();
}

int main()
{
	LED led;

	led.on();
	delay(2000);
	led.off();

	MotorDC drive(GPIO_PIN_0, GPIO_PIN_1), rotate(GPIO_PIN_2, GPIO_PIN_3);

	// Left
//	rotate.run(BACKWARD);
//	delay(500);

	drive.run(FORWARD);
	delay(500);
//	rotate.stop(); // Return
	delay(1000);
	drive.stop();
	delay(500);

	drive.run(BACKWARD);
	delay(1500);
	drive.stop();

	led.on();


	while(1) {}
}
