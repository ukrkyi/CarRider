/* (c) 2020 ukrkyi */
#include "motor_dc.h"

MotorDC::MotorDC(uint16_t posPin, uint16_t negPin) :
	posPin(posPin), negPin(negPin)
{
	__HAL_RCC_GPIOA_CLK_ENABLE();

	HAL_GPIO_WritePin(GPIOA, posPin | negPin, GPIO_PIN_RESET);

	GPIO_InitTypeDef GPIO_InitStruct;

	GPIO_InitStruct.Pin = posPin | negPin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void MotorDC::run(Direction dir)
{
	if (dir == FORWARD) {
		HAL_GPIO_WritePin(GPIOA, negPin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOA, posPin, GPIO_PIN_SET);
	} else {
		HAL_GPIO_WritePin(GPIOA, posPin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOA, negPin, GPIO_PIN_SET);
	}
}

void MotorDC::stop()
{
	HAL_GPIO_WritePin(GPIOA, posPin | negPin, GPIO_PIN_RESET);
}

MotorDC::~MotorDC()
{
	HAL_GPIO_DeInit(GPIOA, posPin | negPin);
}
