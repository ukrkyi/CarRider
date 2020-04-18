/* (c) 2020 ukrkyi */
#ifndef PWM_H
#define PWM_H

#include <stm32f4xx_hal.h>

class PWM
{
	TIM_HandleTypeDef htim;
	uint32_t channel;
public:
	PWM(GPIO_TypeDef * port, uint16_t pin, TIM_TypeDef * timer, uint32_t channel, uint32_t frequency);
	void set(unsigned percent);
	void stop();
	~PWM();
};

#endif // PWM_H
