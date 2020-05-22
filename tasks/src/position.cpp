/* (c) 2020 ukrkyi */
#include "position.h"

#include "system.h"

#include "log.h"

#include "ultrasonic.h"

#include <cstring>

#include "arm_math.h"

#define AXIS_DISTANCE_MM	140

static inline float sqr(const float x) {return x*x;}

Position::Position(const char *name, UBaseType_t priority,
		   I2C &i2c, GPIO_TypeDef *port, uint16_t accReady, uint16_t magReady)
	: Task(name, this, priority), i2c(i2c), port(port), accReady(accReady), magReady(magReady),
	  velocity_forward(0), angle_velocity(0), position_x(0), position_y(0), angle(0)
{
	if (port == GPIOB)
		__HAL_RCC_GPIOB_CLK_ENABLE();
	else
		while (1);

	__HAL_RCC_SYSCFG_CLK_ENABLE();

	GPIO_InitTypeDef GPIO_InitStruct;

	GPIO_InitStruct.Pin = accReady | magReady;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStruct.Pull = GPIO_PULLDOWN;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(port, &GPIO_InitStruct);

	if (accReady == GPIO_PIN_2) {
		accIrq = EXTI2_IRQn;
	} else
		while(1);

	NVIC_SetPriority(accIrq, ACC_DATA_IT_PRIORITY);

	if (magReady == GPIO_PIN_1)
		magIrq = EXTI1_IRQn;
	else
		while(1);

	NVIC_SetPriority(magIrq, MAG_DATA_IT_PRIORITY);

	accel.recalibration = true;
}

void Position::run()
{
	// Test communication
	testConnection();
	// Write configuration
	writeConfig();
	// Check if Interrupt line is already asserted
	if (HAL_GPIO_ReadPin(port, accReady))
		notify(ACC_DATA_AVAILABLE);
	if (HAL_GPIO_ReadPin(port, magReady))
		notify(MAG_DATA_AVAILABLE);
	// calibration routine
	calibrate();
	uint32_t result = 0;
	EventGroup & evt = EventGroup::getInstance();
	Ultrasonic & ranger = Ultrasonic::getInstance();
	evt.notify(POSITION_TASK_READY);
	do {
		// Wait for data available
		result = wait(ACC_DATA_AVAILABLE | MAG_DATA_AVAILABLE);

		if (result & ACC_DATA_AVAILABLE) {
			processAccel();

			evt.notify(POSITION_NEW_DATA);
		}

		if (result & MAG_DATA_AVAILABLE) {
			processMagnet();

			// Update temperature data
			ranger.setTemperature(temp);
		}

		// while processing, new data emerged
		if (HAL_GPIO_ReadPin(port, accReady))
			notify(ACC_DATA_AVAILABLE);
		if (HAL_GPIO_ReadPin(port, magReady))
			notify(MAG_DATA_AVAILABLE);
	} while (1);
}

void Position::startMeasure()
{
	measure = true;
}

void Position::stopMeasure()
{
	measure = false;
}

void Position::testConnection()
{
	uint8_t id = 0;
	i2c.read(addr[ACCELEROMETER], 0x0F, &id, 1, this, I2C_COMM_FINISHED);
	wait(I2C_COMM_FINISHED);
	assert_param(id == 0x41);

	i2c.read(addr[MAGNETOMETER], 0x0F, &id, 1, this, I2C_COMM_FINISHED);
	wait(I2C_COMM_FINISHED);
	assert_param(id == 0x3D);
}

void Position::writeConfig()
{
#ifndef NDEBUG
	uint8_t checkConfig[5];
#endif
	static const uint8_t fifo_rst = 0, fifo_en = 0x20 | accel.fifo_len;
	// Reset FIFO
	i2c.write(addr[ACCELEROMETER], 0x2E, &fifo_rst, 1, this, I2C_COMM_FINISHED);
	wait(I2C_COMM_FINISHED);
	// Enable FIFO, set threshold to accel.fifo_len
	i2c.write(addr[ACCELEROMETER], 0x2E, &fifo_en, 1, this, I2C_COMM_FINISHED);
	wait(I2C_COMM_FINISHED);

	static const uint8_t acc_config[] = {
		0xEF, // Low-pass filter on + ODR 800 HZ + BDU + ALL AXIS ENABLE
		0x40, // Low-pass filter mode 2, High-pass filter off (why someone would ever need it??)
		0x82, // Enable FIFO + FIFO treshold IRQ
		0x04, // Enable address auto-increment
	};
	i2c.write(addr[ACCELEROMETER], 0x20, acc_config, sizeof (acc_config), this, I2C_COMM_FINISHED);

	wait(I2C_COMM_FINISHED);
	NVIC_ClearPendingIRQ(accIrq);
	NVIC_EnableIRQ(accIrq);


#ifndef NDEBUG
	i2c.read(addr[ACCELEROMETER], 0x20, checkConfig, sizeof (acc_config), this, I2C_COMM_FINISHED);
	wait(I2C_COMM_FINISHED);

	assert_param(memcmp(checkConfig, acc_config, 4) == 0);
#endif

	static const uint8_t mag_config[] = {
		0x80, // TEMP SENSOR ON + X,Y AXIS LOW POWER
		0x60, // Not reboot
		0x20, // Enable operation, low-power mode
		0x00, // Z AXIS LOW POWER + BIG-ENDIAN
		0x40, // Enable BDU
	};
	i2c.write(addr[MAGNETOMETER], 0x20 | 0x80, mag_config, sizeof (mag_config), this, I2C_COMM_FINISHED);

	wait(I2C_COMM_FINISHED);
	NVIC_ClearPendingIRQ(magIrq);
	NVIC_EnableIRQ(magIrq);

#ifndef NDEBUG
	i2c.read(addr[MAGNETOMETER], 0x20 | 0x80, checkConfig, sizeof (mag_config), this, I2C_COMM_FINISHED);
	wait(I2C_COMM_FINISHED);
	assert_param(memcmp(checkConfig, mag_config, 5) == 0);
#endif
}

void Position::calibrate()
{
	uint32_t result = 0;
	int calib_samples = 0;
	for (unsigned j = 0; j < 3; j++)
		accel.calib[j] = 0;

	do {
		// Wait for data available
		result = wait(ACC_DATA_AVAILABLE | MAG_DATA_AVAILABLE);

		if (result & MAG_DATA_AVAILABLE) {
			processMagnet();
		}
	}while (!(result & ACC_DATA_AVAILABLE));

	uint8_t status = 0;
	i2c.read(addr[ACCELEROMETER], 0x2F, &status, 1, this, I2C_COMM_FINISHED);

	wait(I2C_COMM_FINISHED);
	if ((status & 0x80) == 0)
		while (1); // How did we get here?...

	i2c.read(addr[ACCELEROMETER], 0x28 | 0x80,
		 (uint8_t *) accel.raw, 6 * accel.fifo_len,
		 this, I2C_COMM_FINISHED);

	wait(I2C_COMM_FINISHED);

	// Drop first sample

	do {
		// Wait for data available
		result = wait(ACC_DATA_AVAILABLE | MAG_DATA_AVAILABLE);

		if (result & ACC_DATA_AVAILABLE) {
			i2c.read(addr[ACCELEROMETER], 0x2F, &status, 1, this, I2C_COMM_FINISHED);

			wait(I2C_COMM_FINISHED);
			if ((status & 0x80) == 0)
				while (1); // How did we get here?...

			i2c.read(addr[ACCELEROMETER], 0x28 | 0x80,
				 (uint8_t *) accel.raw, 6 * accel.fifo_len,
				 this, I2C_COMM_FINISHED);

			wait(I2C_COMM_FINISHED);

			for (size_t i = 0; i < accel.fifo_len; i++)
				for (unsigned j = 0; j < 3; j++)
					accel.calib[j] += accel.raw[i*3 + j];
			calib_samples += accel.fifo_len;
		}

		if (result & MAG_DATA_AVAILABLE) {
			processMagnet();
		}

		// while processing, new data emerged
		if (HAL_GPIO_ReadPin(port, accReady))
			notify(ACC_DATA_AVAILABLE);
		if (HAL_GPIO_ReadPin(port, magReady))
			notify(MAG_DATA_AVAILABLE);
	} while (calib_samples != 800);
	// We got our 800 samples
	for (unsigned j = 0; j < 3; j++)
		accel.calib[j] /= calib_samples;
}

void Position::processAccel()
{
	// sum in static state for recalibration
	static int32_t recalib_sum[3];
	// number of samples gathered for recalibration
	static int recalib_samples;
	// number of samples collected
	static int samples_collected;
	// sum of samples collected
	static int32_t samples_sum[3];
	// Number of static periods
	static size_t zero_periods;

	// Zero step: read data out
	// status register
	uint8_t status = 0;

	i2c.read(addr[ACCELEROMETER], 0x2F, &status, 1, this, I2C_COMM_FINISHED);
	wait(I2C_COMM_FINISHED);
	if ((status & 0x80) == 0)
		while (1); // How did we get here?...

	i2c.read(addr[ACCELEROMETER], 0x28 | 0x80,
		 (uint8_t *) accel.raw, 6 * accel.fifo_len, this, I2C_COMM_FINISHED);
	wait(I2C_COMM_FINISHED);

	// First step: compensation
	for (size_t i = 0; i < accel.fifo_len; i++) {
		// Process every axis
		for (unsigned j = 0; j < 3; j++)
			// Add compensated data
			samples_sum[j] += accel.raw[i*3 + j] - accel.calib[j];
	}
	samples_collected += accel.fifo_len;

	// Second step: averaging
	if (samples_collected == accel.average_samples) {
		int16_t averaged_acceleration[3];
		// We got the number of samples needed to perform average
		zero_periods++; // increase in case all samples are zero
		for (unsigned j = 0; j < 3; j++) {
			if (abs(samples_sum[j] / samples_collected) >= accel.discrimination_window[j]) {
				averaged_acceleration[j] = samples_sum[j] / samples_collected;
				zero_periods = 0;
			} else {
				averaged_acceleration[j] = 0;
			}
			samples_sum[j] = 0;
		}
		samples_collected = 0;

		// Calculate acceleration in millimeters per second^2
		// NOTE should we handle this uniquely?
		float acceleration_x = averaged_acceleration[X] * 1000 * 10.f / accel.calib[Z];
		float acceleration_y = averaged_acceleration[Y] * 10.f * 0.061f;

		static const float discretization = accel.average_samples / 800.f;

		float acceleration_forward = 0, acceleration_center = 0;
		float momental_velocity = 0, momental_angle_v = 0;
		float prev_delta = 0;
		float delta = 0;
		do {
			acceleration_forward = acceleration_x * cosf(delta) + acceleration_y * sinf(delta);
			acceleration_center  = acceleration_y * cosf(delta) - acceleration_x * sinf(delta);

			if (acceleration_y && signbit(acceleration_center) != signbit(acceleration_y)){
				// Actually, we should never reach it
				// But it can be possible if delta sign differs from y acceleration sign
				Log::getInstance().write("Error: sign of accelerations differ");
				while (1) {
					vTaskDelay(1); // Allow data to be transmitted for debug purposes
				}
			}

			if (zero_periods >= 2)
				momental_velocity = 0;
			else
				momental_velocity = velocity_forward + acceleration_forward * discretization;

			if (fabsf(momental_velocity) >= 250 || delta != 0)
				momental_angle_v = acceleration_center / momental_velocity;
			else
				momental_angle_v = 0;

			prev_delta = delta;

			if (momental_angle_v != 0)
				delta = copysignf(atanf(2/sqrtf(sqr(4*momental_velocity/momental_angle_v/AXIS_DISTANCE_MM)-1)), momental_angle_v);
			else
				delta = 0;

			// TODO set up maximum iterations number
		} while (fabsf(delta - prev_delta) >= 0.001);

		if (isnanf(delta)){
			Log::getInstance().write("Delta is nan\n");

			delta = 0;
			acceleration_forward = acceleration_x;
			acceleration_center  = acceleration_y;

			if (zero_periods >= 2)
				momental_velocity = 0;
			else
				momental_velocity = velocity_forward + acceleration_forward * discretization;

			if (fabsf(momental_velocity) >= 250)
				momental_angle_v = acceleration_center / momental_velocity;
			else
				momental_angle_v = 0;
		}

		float beta = atanf(tanf(delta) / 2);
		position_x += (velocity_forward + momental_velocity) * cosf(angle + beta) * discretization / 2;
		position_y += (velocity_forward + momental_velocity) * sinf(angle + beta) * discretization / 2;
		angle      += (angle_velocity + momental_angle_v) * discretization / 2;

		velocity_forward = momental_velocity;
		angle_velocity = momental_angle_v;

		if (measure) {
			Log & log = Log::getInstance();
			log.write("%d,%d.%d,%d.%d,%d.%d",
				  xTaskGetTickCount(),
				  (int) position_x,       abs((int) (position_x       * 1000) % 1000),
				  (int) position_y,       abs((int) (position_y       * 1000) % 1000),
				  (int) (angle * 180/PI), abs((int) (angle * 180 / PI * 1000) % 1000));

			log.write("\n");
		}
	}

	// Recalibration
	if (accel.recalibration) {
		if (zero_periods >= 2) {
			for (size_t i = 0; i < accel.fifo_len; i++)
				for (unsigned j = 0; j < 3; j++)
					recalib_sum[j] += accel.raw[i*3 + j];
			recalib_samples += accel.fifo_len;

			if (recalib_samples == 800) {
				for (unsigned j = 0; j < 3; j++){
					accel.calib[j] = (accel.calib[j] + recalib_sum[j] / recalib_samples) / 2;
					recalib_sum[j] = 0;
				}
				recalib_samples = 0;
			}
		} else {
			recalib_samples = 0;
			for (unsigned j = 0; j < 3; j++)
				recalib_sum[j] = 0;
		}
	}
}

void Position::setZeroPosition()
{
	position_x = 0;
	position_y = 0;
	angle = 0;
}

void Position::processMagnet()
{
	uint8_t status = 0;
	i2c.read(addr[MAGNETOMETER], 0x27, &status, 1, this, I2C_COMM_FINISHED);

	wait(I2C_COMM_FINISHED);
	if ((status & 0x08) == 0)
		while (1); // How did we get here?...

	i2c.read(addr[MAGNETOMETER], 0x28 | 0x80,
		 (uint8_t *) magnet_raw, 6,
		 this, I2C_COMM_FINISHED);

	wait(I2C_COMM_FINISHED);

	int16_t temp_raw = 0;
	i2c.read(addr[MAGNETOMETER], 0x2E | 0x80, (uint8_t *) &temp_raw, 2,
		 this, I2C_COMM_FINISHED);
	wait(I2C_COMM_FINISHED);

	temp = ((float) temp_raw) / 8 + 25;
}

Position &Position::getInstance()
{
	static Position pos("position", POSITION_TASK_PRIORITY, I2C::getInstance(),
			    GPIOB, GPIO_PIN_2, GPIO_PIN_1);
	return pos;
}

void Position::processAccIrq()
{
	assert_param(__HAL_GPIO_EXTI_GET_IT(accReady));
	__HAL_GPIO_EXTI_CLEAR_IT(accReady);
	notifyISR(ACC_DATA_AVAILABLE);
}

void Position::processMagIrq()
{
	assert_param(__HAL_GPIO_EXTI_GET_IT(magReady));
	__HAL_GPIO_EXTI_CLEAR_IT(magReady);
	notifyISR(MAG_DATA_AVAILABLE);
}
