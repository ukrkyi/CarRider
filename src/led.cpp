/* (c) 2020 ukrkyi */
#include "led.h"

#include <stm32f4xx_hal.h>

LED::LED()
{
	__HAL_RCC_GPIOC_CLK_ENABLE();

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
