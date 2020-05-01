/* (c) 2020 ukrkyi */
#ifndef ULTRASONIC_H
#define ULTRASONIC_H

#include <stm32f4xx_hal.h>

class Ultrasonic
{
	TIM_HandleTypeDef trigTim, echoTim;
	uint32_t trigCh, echoCh;
	volatile float distance;
	Ultrasonic(GPIO_TypeDef * trigPort, uint16_t trigPin,
		   GPIO_TypeDef * echoPort, uint16_t echoPin,
		   TIM_TypeDef * trigTimer, uint32_t trigChannel,
		   TIM_TypeDef * echoTimer, uint32_t echoChannel, uint32_t period);
public:
	static Ultrasonic& getInstance();
	Ultrasonic(const Ultrasonic&) = delete;
	Ultrasonic() = delete;
	void start();
	void stop();
	void processEcho(float avgSpeed); // meters per us
	float getDistance() const;
};

#endif // ULTRASONIC_H
