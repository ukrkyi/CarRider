/* (c) 2020 ukrkyi */
#include "led.h"

#include <stm32f4xx_hal.h>

#include "FreeRTOS.h"
#include "task.h"

LED::LED()
{
	__HAL_RCC_GPIOC_CLK_ENABLE();

	off();

	GPIO_InitTypeDef GPIO_InitStruct;

	GPIO_InitStruct.Pin = GPIO_PIN_13;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

LED::~LED()
{
	HAL_GPIO_DeInit(GPIOC, GPIO_PIN_13);
}

void LED::blink(unsigned n, unsigned wait_till)
{
	const unsigned t = 250;
	for (unsigned i = 0; i < n; i++) {
		on();
		vTaskDelay(t);
		off();
		vTaskDelay(t);
	}
	if (wait_till > n)
		vTaskDelay(t * 2 * (wait_till - n));
}

void LED::on()
{
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
}

void LED::off()
{
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
}

void LED::toggle()
{
	HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
}

bool LED::is_on()
{
	return HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_RESET;
}

LED &LED::getInstance()
{
	static LED led;
	return led;
}
