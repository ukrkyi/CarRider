/* (c) 2020 ukrkyi */
#ifndef PWM_H
#define PWM_H

#include <stm32f4xx_hal.h>

enum PWMChannel {
	MOTOR_1_FW,
	MOTOR_1_BW,
	MOTOR_2_FW,
	MOTOR_2_BW,
	PWM_CHANNEL_NUM
};

class PWM
{
	TIM_HandleTypeDef htim_base;
	TIM_HandleTypeDef * htim;
	uint32_t channel;
	void InitPin(GPIO_TypeDef * port, uint16_t pin);
	void InitChannel();
	PWM(GPIO_TypeDef * port, uint16_t pin, TIM_TypeDef * timer, uint32_t channel, uint32_t frequency);
	PWM(GPIO_TypeDef * port, uint16_t pin, uint32_t channel, PWM& timerBase);
	~PWM();
public:
	PWM(const PWM&) = delete;
	PWM() = delete;
	void set(unsigned percent);
	void stop();
	static PWM& getInstance(PWMChannel channel);
};

#endif // PWM_H
