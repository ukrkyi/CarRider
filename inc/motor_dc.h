/* (c) 2020 ukrkyi */
#ifndef MOTORDC_H
#define MOTORDC_H

#include <stm32f4xx_hal.h>

enum Direction {
	FORWARD = 0,
	BACKWARD
};

class MotorDC
{
	uint16_t posPin, negPin;
public:
	MotorDC(uint16_t posPin, uint16_t negPin);
	void run(Direction dir);
	void stop();
	~MotorDC();
};

#endif // MOTORDC_H
