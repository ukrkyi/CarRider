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

void blink(LED& led, int n) {
	for (int i = 0; i < n; i++) {
		led.on();
		delay(250);
		led.off();
		delay(250);
	}
}

int main()
{
	LED led;

	led.on();
	delay(2000);
	led.off();

	__HAL_RCC_GPIOA_CLK_ENABLE();

	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);

	GPIO_InitTypeDef GPIO_InitStruct;

	GPIO_InitStruct.Pin = GPIO_PIN_1;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	PWM motor1(GPIOA, GPIO_PIN_0, TIM5, TIM_CHANNEL_1, 80000);

	for (int i = 50; i > 0; i -= 10){
		blink(led, i / 10);
		motor1.set(i);
		delay(1000);
	}

	motor1.stop();

	//MotorDC drive(GPIO_PIN_0, GPIO_PIN_1), rotate(GPIO_PIN_2, GPIO_PIN_3);

	led.on();


	while(1) {}
}
