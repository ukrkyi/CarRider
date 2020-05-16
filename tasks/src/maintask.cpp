/* (c) 2020 ukrkyi */
#include "maintask.h"

// Tasks
#include "wifi.h"
#include "position.h"

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
	const WiFi::Socket comp = { "10.42.0.1", 1488 };
	const WiFi::AccessPoint ap = { "ukrkyi-hotspot", "qwe123QWE!@#", false };

	WiFi::Data data;

	LED & led = LED::getInstance();
	WiFi & wifi = WiFi::getInstance();

	EventGroup &evt = EventGroup::getInstance();

	evt.clear(WIFI_COMMAND_ERROR);

	Event res;

	wifi.sendCommand(WiFi::POWER_ON);

	res = evt.wait(WIFI_COMMAND_ERROR | WIFI_STATE_CHANGED);

	if (res & WIFI_COMMAND_ERROR)
		while (1);         // TODO handle error

	vTaskDelay(1000);

	//	while (wifi.getState() != WiFi::WIFI_CONNECTED)
	//		evt.wait(WIFI_STATE_CHANGED);

	if (wifi.getState() != WiFi::WIFI_CONNECTED) {
		wifi.sendCommand(WiFi::WIFI_CONNECT, (void*)&ap);

		res = evt.wait(WIFI_COMMAND_ERROR | WIFI_STATE_CHANGED);

		if (res & WIFI_COMMAND_ERROR)
			while (1);         // TODO handle error
	}

	do {
		wifi.sendCommand(WiFi::TCP_CONNECT, (void*)&comp);
		res = evt.wait(WIFI_COMMAND_ERROR | WIFI_STATE_CHANGED);

		if (res & WIFI_COMMAND_ERROR) {
			led.on();
			vTaskDelay(1000);
			continue;
		}
	} while (wifi.getState() != WiFi::TCP_CONNECTED);

	led.off();

	wifi.sendCommand(WiFi::TCP_SEND, &(data = { 3, "o/\n" }));

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
