/* (c) 2020 ukrkyi */
#ifndef POSITION_H
#define POSITION_H

#include "task.hpp"

#include <stm32f4xx_hal.h>

#include "i2c.h"

class Position : public Task
{
	enum Notification {
		NONE = 0,
		I2C_COMM_FINISHED = 0x01,
		ACC_DATA_AVAILABLE = 0x10,
		MAG_DATA_AVAILABLE = 0x20,
	};

	enum Sensor {
		ACCELEROMETER,
		MAGNETOMETER
	};

	I2C& i2c;

	const uint8_t addr[2] = {
		[ACCELEROMETER] = 0x1D,
		[MAGNETOMETER] = 0x1E
	};

	int16_t accel_raw[30], magnet_raw[3];

	float temp;

	IRQn_Type accIrq, magIrq;
	GPIO_TypeDef * port;
	uint16_t accReady, magReady;

	bool measure;
	int time;

	Position(const char * name, UBaseType_t priority,
		 I2C& i2c, GPIO_TypeDef * port, uint16_t accReady, uint16_t magReady);
	void run();

	void testConnection();
	void writeConfig();

	bool readData(Sensor sensor);

public:

	static Position& getInstance();

	void processAccIrq();
	void processMagIrq();

	void startMeasure();
	void stopMeasure();
};

#endif // POSITION_H
