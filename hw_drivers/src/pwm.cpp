/* (c) 2020 ukrkyi */
#include "pwm.h"

void PWM::InitPin(GPIO_TypeDef *port, uint16_t pin)
{
	if (port == GPIOA)
		__HAL_RCC_GPIOA_CLK_ENABLE();
	else
		while(1);

	GPIO_InitTypeDef gpio;

	gpio.Pin = pin;
	gpio.Mode = GPIO_MODE_AF_PP;
	gpio.Pull = GPIO_NOPULL;
	gpio.Speed = GPIO_SPEED_FREQ_MEDIUM;
	gpio.Alternate = GPIO_AF2_TIM5;
	HAL_GPIO_Init(port, &gpio);
}

void PWM::InitChannel()
{
	TIM_OC_InitTypeDef chConf;
	chConf.OCMode = TIM_OCMODE_PWM1;
	chConf.Pulse = 0;
	chConf.OCPolarity = TIM_OCPOLARITY_HIGH;
	chConf.OCFastMode = TIM_OCFAST_DISABLE;
	HAL_TIM_PWM_ConfigChannel(htim, &chConf, channel);

	HAL_TIM_PWM_Start(htim, channel);
	TIM_CCxChannelCmd(htim->Instance, channel, TIM_CCx_DISABLE);
}

PWM::PWM(GPIO_TypeDef *port, uint16_t pin, TIM_TypeDef *timer, uint32_t channel, uint32_t frequency)
	: channel(channel)
{
	InitPin(port, pin);

	// Manage timer

	if (timer == TIM5)
		__HAL_RCC_TIM5_CLK_ENABLE();
	else
		while(1);

	htim_base.Instance = timer;
	htim_base.Init.Prescaler = SystemCoreClock/(frequency *= 100) - 1;
	htim_base.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim_base.Init.Period = 99;
	htim_base.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim_base.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
	HAL_TIM_PWM_Init(&htim_base);

	htim = &htim_base;

	InitChannel();
}

PWM::PWM(GPIO_TypeDef *port, uint16_t pin, uint32_t channel, PWM &timerBase) :
	channel(channel)
{
	InitPin(port, pin);

	htim_base.Instance = NULL;
	htim = timerBase.htim;

	InitChannel();
}

void PWM::set(unsigned percent)
{
	__HAL_TIM_SET_COMPARE(htim, channel, percent);
	TIM_CCxChannelCmd(htim->Instance, channel, TIM_CCx_ENABLE);
}

void PWM::stop()
{
	TIM_CCxChannelCmd(htim->Instance, channel, TIM_CCx_DISABLE);
}

PWM &PWM::getInstance(PWMChannel channel)
{
	static PWM pwm_motor_1(GPIOA, GPIO_PIN_0, TIM5, TIM_CHANNEL_1, 160000),
		pwm_motor_2(GPIOA, GPIO_PIN_1, TIM_CHANNEL_2, pwm_motor_1),
		pwm_motor_3(GPIOA, GPIO_PIN_2, TIM_CHANNEL_3, pwm_motor_1),
		pwm_motor_4(GPIOA, GPIO_PIN_3, TIM_CHANNEL_4, pwm_motor_1);
	switch (channel) {
	case MOTOR_1_FW:
		return pwm_motor_1;
	case MOTOR_1_BW:
		return pwm_motor_2;
	case MOTOR_2_FW:
		return pwm_motor_3;
	case MOTOR_2_BW:
		return pwm_motor_4;
	default:
		while(1);
	}
}

PWM::~PWM()
{
	if (htim_base.Instance != NULL)
		HAL_TIM_PWM_DeInit(&htim_base);
}
