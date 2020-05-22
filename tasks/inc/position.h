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

	enum Axis {
		X = 0,
		Y = 1,
		Z = 2
	};

	I2C& i2c;

	const uint8_t addr[2] = {
		[ACCELEROMETER] = 0x1D,
		[MAGNETOMETER] = 0x1E
	};

	int16_t magnet_raw[3];

	struct AccelData {
		/** Number of samples in burst read */
		static const size_t fifo_len = 20;
		// raw data
		int16_t raw[fifo_len * 3];
		// calibration values
		int32_t calib[3];
		// is recalibration active?
		bool recalibration;
		// discrimination window for axis
		const int16_t discrimination_window[3] = {200, 240, 80};
		/** Number of samples saved for averaging.
		 *  Should be multiple of @ref fifo_len */
		static const int average_samples = 80;
	} accel;

	IRQn_Type accIrq, magIrq;
	GPIO_TypeDef * port;
	uint16_t accReady, magReady;

	bool measure;

	float velocity_forward;
	float angle_velocity;
	float position_x;
	float position_y;
	float angle;

	float temp;

	void testConnection();
	void writeConfig();
	void calibrate();

	void processAccel();
	void processMagnet();

	Position(const char * name, UBaseType_t priority,
		 I2C& i2c, GPIO_TypeDef * port, uint16_t accReady, uint16_t magReady);
	void run();

public:

	static Position& getInstance();

	void processAccIrq();
	void processMagIrq();

	void startMeasure();
	void stopMeasure();

	void enableRecalibration();
	void disableRecalibration();

	void setZeroPosition();
};

#endif // POSITION_H
