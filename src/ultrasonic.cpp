/* (c) 2020 ukrkyi */
#include "ultrasonic.h"

#define COUNTER_FREQ 1000000

Ultrasonic::Ultrasonic(GPIO_TypeDef *trigPort, uint16_t trigPin, GPIO_TypeDef *echoPort, uint16_t echoPin, TIM_TypeDef *trigTimer, uint32_t trigChannel, TIM_TypeDef *echoTimer, uint32_t echoChannel, uint32_t period)
	: trigCh(trigChannel), echoCh(echoChannel)
{
	if (trigPort == GPIOA || echoPort == GPIOA)
		__HAL_RCC_GPIOA_CLK_ENABLE();

	if (trigPort == GPIOB || echoPort == GPIOB)
		__HAL_RCC_GPIOB_CLK_ENABLE();

	if (trigTimer == TIM1 || echoTimer == TIM1)
		__HAL_RCC_TIM1_CLK_ENABLE();
	if (trigTimer == TIM2 || echoTimer == TIM2)
		__HAL_RCC_TIM2_CLK_ENABLE();
	if (trigTimer == TIM3 || echoTimer == TIM3)
		__HAL_RCC_TIM3_CLK_ENABLE();
	if (trigTimer == TIM4 || echoTimer == TIM4)
		__HAL_RCC_TIM4_CLK_ENABLE();

	HAL_GPIO_WritePin(trigPort, trigPin, GPIO_PIN_RESET);

	GPIO_InitTypeDef gpio;

	gpio.Pin = trigPin;
	gpio.Mode = GPIO_MODE_AF_PP;
	gpio.Pull = GPIO_NOPULL;
	gpio.Speed = GPIO_SPEED_FREQ_LOW;
	if (trigTimer == TIM3)
		gpio.Alternate = GPIO_AF2_TIM3;
	else
		while(1);
	HAL_GPIO_Init(trigPort, &gpio);

	gpio.Pin = echoPin;
	gpio.Mode = GPIO_MODE_AF_PP;
	gpio.Pull = GPIO_PULLDOWN;
	if (echoTimer == TIM2)
		gpio.Alternate = GPIO_AF1_TIM2;
	else
		while(1);
	HAL_GPIO_Init(echoPort, &gpio);

	if (period < 60000) // if period less then 60 mS
		while(1);

	trigTim.Instance = trigTimer;
	trigTim.Init.Prescaler = SystemCoreClock/100000 - 1; // 10 uS
	trigTim.Init.CounterMode = TIM_COUNTERMODE_UP;
	trigTim.Init.Period = period/10 - 1;
	trigTim.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	trigTim.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
	HAL_TIM_PWM_Init(&trigTim);

	TIM_OC_InitTypeDef chConf;
	chConf.OCMode = TIM_OCMODE_PWM1;
	chConf.Pulse = 1;
	chConf.OCPolarity = TIM_OCPOLARITY_HIGH;
	chConf.OCFastMode = TIM_OCFAST_DISABLE;
	chConf.OCNPolarity = TIM_OCNPOLARITY_HIGH;
	chConf.OCIdleState = TIM_OCIDLESTATE_RESET;
	chConf.OCNIdleState = TIM_OCNIDLESTATE_RESET;
	HAL_TIM_PWM_ConfigChannel(&trigTim, &chConf, trigChannel);

	echoTim.Instance = echoTimer;
	echoTim.Init.Prescaler = SystemCoreClock/1000000 - 1; // running at 1 uS
	echoTim.Init.CounterMode = TIM_COUNTERMODE_UP;
	echoTim.Init.Period = period - 1001;// Count till 1 mS before next tick
	echoTim.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	echoTim.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
	HAL_TIM_IC_Init(&echoTim);

	TIM_SlaveConfigTypeDef slConf;
	slConf.SlaveMode = TIM_SLAVEMODE_RESET;
	slConf.InputTrigger = TIM_TS_ETRF;
	slConf.TriggerPolarity = TIM_ETRPOLARITY_NONINVERTED;
	slConf.TriggerPrescaler = TIM_ETRPRESCALER_DIV1;
	slConf.TriggerFilter = 0x02;
	HAL_TIM_SlaveConfigSynchro(&echoTim, &slConf);

	TIM_IC_InitTypeDef icConf;
	icConf.ICPolarity = TIM_ICPOLARITY_FALLING;
	icConf.ICSelection= TIM_ICSELECTION_DIRECTTI;
	icConf.ICPrescaler= TIM_ICPSC_DIV1;
	icConf.ICFilter   = 0x02;
	HAL_TIM_IC_ConfigChannel(&echoTim, &icConf, echoChannel);
}

void Ultrasonic::start()
{
	HAL_TIM_PWM_Start(&trigTim, trigCh);
	HAL_TIM_IC_Start_IT(&echoTim, echoCh);
}

void Ultrasonic::stop()
{
	HAL_TIM_PWM_Stop(&trigTim, trigCh);
	HAL_TIM_IC_Stop_IT(&echoTim, echoCh);
}

float Ultrasonic::processEcho(int distanceTravelled)
{
	if (echoCh == TIM_CHANNEL_1 &&
	    __HAL_TIM_GET_IT_SOURCE(&echoTim, TIM_IT_CC1)) {
		if (__HAL_TIM_GET_FLAG(&echoTim, TIM_FLAG_CC1)) // Capture occured
			__HAL_TIM_CLEAR_IT(&echoTim, TIM_IT_CC1);
		else if (__HAL_TIM_GET_IT_SOURCE(&echoTim, TIM_FLAG_CC1OF)) // Overcapture is sad((((
			while(1);
	} else
		while(1);
	uint32_t us = __HAL_TIM_GET_COMPARE(&echoTim, echoCh);
	const uint32_t temp = +20;
	const float soundSpeed = ((float)(331300+596*temp))/1000000;
	return (us*soundSpeed - distanceTravelled)/2;
}
