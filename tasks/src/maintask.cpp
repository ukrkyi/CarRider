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


	//	Ultrasonic & range = Ultrasonic::getInstance();

	//	range.start();

	//	static float distance;
	//	static char str[25];
	//	static unsigned size;

	//	while(1) {
	//		evt.wait(ULTRASONIC_NEW_DATA);

	//		distance = range.getDistance();
	//		size = snprintf(str, 25, "Ultrasonic: %d.%d mm\n", (int) distance, (int) (distance * 10) % 10);
	//		data = {size, str};

	//		evt.clear(WIFI_CMD_PROCESSED);
	//		wifi.sendCommand(WiFi::TCP_SEND, &data);
	//		evt.wait(WIFI_CMD_PROCESSED);
	//		vTaskDelay(5000);
	//	}

	EventGroup::getInstance().wait(LOG_TASK_READY);
	EventGroup::getInstance().wait(POSITION_TASK_READY);

	Position & pos = Position::getInstance();

	pos.startMeasure();

	vTaskDelay(1000);

	drive.run(70);
	vTaskDelay(2000);

	turn.run(FORWARD);
	vTaskDelay(1000);

	turn.stop();
	vTaskDelay(1000);

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
