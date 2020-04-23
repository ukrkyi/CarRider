/* (c) 2020 ukrkyi */
#ifndef PWM_H
#define PWM_H

#include <stm32f4xx_hal.h>

class PWM
{
	TIM_HandleTypeDef htim_base;
	TIM_HandleTypeDef * htim;
	uint32_t channel;
	void InitPin(GPIO_TypeDef * port, uint16_t pin);
	void InitChannel();
public:
	PWM(GPIO_TypeDef * port, uint16_t pin, TIM_TypeDef * timer, uint32_t channel, uint32_t frequency);
	PWM(GPIO_TypeDef * port, uint16_t pin, uint32_t channel, PWM& timerBase);
	PWM(const PWM&) = delete;
	PWM() = delete;
	void set(unsigned percent);
	void stop();
	~PWM();
};

#endif // PWM_H
