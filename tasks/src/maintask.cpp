/* (c) 2020 ukrkyi */
#include "maintask.h"

// Tasks
#include "wifi.h"
#include "position.h"
#include "log.h"

//Drivers
#include "motor_dc.h"
#include "led.h"
#include "ultrasonic.h"

// OS
#include "eventgroup.h"

Main::Main(const char *name, UBaseType_t priority) :
	Task(name, this, priority)
{

}

void Main::run()
{
	//	LED& led = LED::getInstance();

	MotorDC & drive = MotorDC::getInstance(MOTOR_DRIVE),
			&turn = MotorDC::getInstance(MOTOR_TURN);

	EventGroup & evt = EventGroup::getInstance();

	evt.wait(LOG_TASK_READY);
	evt.wait(POSITION_TASK_READY);

	Ultrasonic & range = Ultrasonic::getInstance();
	range.start();

	Position & pos = Position::getInstance();
	pos.startMeasure();

	vTaskDelay(1000);

	drive.run(70);

	uint32_t count = xTaskGetTickCount();
	static float distance;
	while (xTaskGetTickCount() - count < 4000) {
		evt.wait(ULTRASONIC_NEW_DATA);

		distance = range.getDistance();

		if (distance > 50 && distance < 500) {
			drive.stop();
			Log::getInstance().write("Error: object detected at %d mm.\n", (int) distance);
			break;
		}

		if (xTaskGetTickCount() - count >= 3000)
			turn.stop();
		else if (xTaskGetTickCount() - count >= 2000)
			turn.run(BACKWARD);
		else if (xTaskGetTickCount() - count >= 1000)
			turn.run(FORWARD);
	}

	drive.stop();
	vTaskDelay(2000);

	pos.stopMeasure();

	while (1)
		vTaskDelay(1);
}

Main &Main::getInstance()
{
	static Main maintask("main", MAIN_TASK_PRIORITY);

	return maintask;
}
