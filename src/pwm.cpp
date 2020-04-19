/* (c) 2020 ukrkyi */
#include "pwm.h"

PWM::PWM(GPIO_TypeDef *port, uint16_t pin, TIM_TypeDef *timer, uint32_t channel, uint32_t frequency)
	: channel(channel)
{
	if (port == GPIOA)
		__HAL_RCC_GPIOA_CLK_ENABLE();
	else
		while(1);

	if (timer == TIM5)
		__HAL_RCC_TIM5_CLK_ENABLE();
	else
		while(1);

	GPIO_InitTypeDef gpio;

	gpio.Pin = pin;
	gpio.Mode = GPIO_MODE_AF_PP;
	gpio.Pull = GPIO_NOPULL;
	gpio.Speed = GPIO_SPEED_FREQ_MEDIUM;
	gpio.Alternate = GPIO_AF2_TIM5;
	HAL_GPIO_Init(port, &gpio);

	htim.Instance = timer;
	htim.Init.Prescaler = SystemCoreClock/(frequency *= 100) - 1;
	htim.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim.Init.Period = 99;
	htim.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
	HAL_TIM_PWM_Init(&htim);

	TIM_OC_InitTypeDef chConf;
	chConf.OCMode = TIM_OCMODE_PWM1;
	chConf.Pulse = 0;
	chConf.OCPolarity = TIM_OCPOLARITY_HIGH;
	chConf.OCFastMode = TIM_OCFAST_DISABLE;
	HAL_TIM_PWM_ConfigChannel(&htim, &chConf, channel);

	HAL_TIM_PWM_Start(&htim, channel);
	TIM_CCxChannelCmd(htim.Instance, channel, TIM_CCx_DISABLE);
}

void PWM::set(unsigned percent)
{
	__HAL_TIM_SET_COMPARE(&htim, channel, percent);
	TIM_CCxChannelCmd(htim.Instance, channel, TIM_CCx_ENABLE);
}

void PWM::stop()
{
	TIM_CCxChannelCmd(htim.Instance, channel, TIM_CCx_DISABLE);
}

PWM::~PWM()
{
	HAL_TIM_PWM_DeInit(&htim);
}