/* (c) 2020 ukrkyi */
#include "motor_dc.h"

MotorDC::MotorDC(PWM& forward, PWM& backward) :
	forward(forward), backward(backward)
{
	forward.stop();
	backward.stop();
}

void MotorDC::run(int speed)
{
	if (speed > 0) {
		backward.stop();
		forward.set(speed);
	} else if (speed < 0) {
		forward.stop();
		backward.set(-speed);
	} else {
		stop();
	}
}

void MotorDC::stop()
{
	forward.stop();
	backward.stop();
}

MotorDC::~MotorDC()
{
	stop();
}
