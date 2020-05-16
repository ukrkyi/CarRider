/* (c) 2020 ukrkyi */
#ifndef MOTORDC_H
#define MOTORDC_H

#include <stm32f4xx_hal.h>
#include "pwm.h"

#include "mutex.h"

enum Direction {
	FORWARD = 100,
	BACKWARD = -100
};

enum Motor {
	MOTOR_DRIVE,
	MOTOR_TURN,
	MOTOR_NUM
};

class MotorDC
{
	PWM &forward, &backward;
	Mutex mutex;
	MotorDC(PWM& forward, PWM& backward);
public:
	static MotorDC& getInstance(Motor motor);
	MotorDC(const MotorDC&) = delete;
	MotorDC() = delete;
	void run(int speed);
	void stop();
	~MotorDC();
};

#endif // MOTORDC_H
