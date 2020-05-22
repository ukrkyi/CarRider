/* (c) 2020 ukrkyi */
#include "motor_dc.h"

MotorDC::MotorDC(PWM& forward, PWM& backward) :
	forward(forward), backward(backward)
{
	forward.stop();
	backward.stop();
}

MotorDC &MotorDC::getInstance(Motor motor)
{
	static MotorDC drive(PWM::getInstance(MOTOR_1_FW), PWM::getInstance(MOTOR_1_BW)),
			turn(PWM::getInstance(MOTOR_2_FW), PWM::getInstance(MOTOR_2_BW));
	switch (motor) {
	case MOTOR_DRIVE:
		return drive;
	case MOTOR_TURN:
		return turn;
	default:
		while(1);
	}
}

void MotorDC::run(int speed)
{
	if (!mutex.isAlreadyLocked())
		mutex.lock();
	if (speed > 0) {
		backward.stop();
		forward.set(speed);
	} else if (speed < 0) {
		forward.stop();
		backward.set(-speed);
	} else {
		forward.stop();
		backward.stop();
	}
}

void MotorDC::stop()
{
	forward.stop();
	backward.stop();
	mutex.unlock();
}

MotorDC::~MotorDC()
{
	stop();
}
