/* (c) 2020 ukrkyi */
#ifndef MOTORDC_H
#define MOTORDC_H

#include <stm32f4xx_hal.h>
#include "pwm.h"

enum Direction {
	FORWARD = 100,
	BACKWARD = -100
};

class MotorDC
{
	PWM &forward, &backward;
public:
	MotorDC(PWM& forward, PWM& backward);
	void run(int speed);
	void stop();
	~MotorDC();
};

#endif // MOTORDC_H
