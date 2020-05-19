/* (c) 2020 ukrkyi */
#include "position.h"

#include "system.h"

#include "log.h"

#include <cstring>

Position::Position(const char *name, UBaseType_t priority,
		   I2C &i2c, GPIO_TypeDef *port, uint16_t accReady, uint16_t magReady)
	: Task(name, this, priority), i2c(i2c), port(port), accReady(accReady), magReady(magReady)
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
	uint32_t result = 0;
	Log & log = Log::getInstance();
	EventGroup & evt = EventGroup::getInstance();
	evt.notify(POSITION_TASK_READY);
	do {
		// Wait for data available
		result = wait(ACC_DATA_AVAILABLE | MAG_DATA_AVAILABLE);

		if (result & ACC_DATA_AVAILABLE) {
			while (!readData(ACCELEROMETER));
			// TODO process accelerometer data
			if (measure) {
				for (int i = 0; i < 20; i++)
					log.write("ACC,%d,%d,%d,%d\n", time, accel_raw[i*3], accel_raw[i*3 + 1], accel_raw[i*3 + 2]);
			}
		}

		if (result & MAG_DATA_AVAILABLE) {
			while (!readData(MAGNETOMETER));
			// TODO process magnetometer data
			if (measure) {
				log.write("MAG,%d,%d,%d,%d\n",
					  time, magnet_raw[0], magnet_raw[1], magnet_raw[2]);
			}
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
	static uint8_t fifo_rst = 0, fifo_en = 0x20 | 20;
	// Reset FIFO
	i2c.write(addr[ACCELEROMETER], 0x2E, &fifo_rst, 1, this, I2C_COMM_FINISHED);
	wait(I2C_COMM_FINISHED);
	// Enable FIFO, set threshold to 20
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
		0xFC, // X, Y AXIS HIGH RESOLUTION + ODR 80 HZ + TEMP SENSOR ON
		0x60, // Not reboot
		0x00, // Enable operation
		0x0C, // Z AXIS HIGH RESOLUTION + BIG-ENDIAN
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

bool Position::readData(Position::Sensor sensor)
{
	uint8_t status = 0;
	if (sensor == MAGNETOMETER) {
		i2c.read(addr[sensor], 0x27, &status, 1, this, I2C_COMM_FINISHED);

		wait(I2C_COMM_FINISHED);
		if ((status & 0x08) == 0)
			while (1); // How did we get here?...
	} else { //Accel
		i2c.read(addr[sensor], 0x2F, &status, 1, this, I2C_COMM_FINISHED);

		wait(I2C_COMM_FINISHED);
		if ((status & 0x80) == 0)
			while (1); // How did we get here?...
	}

	time = xTaskGetTickCount();

	i2c.read(addr[sensor], 0x28 | 0x80,
		 (uint8_t *) (sensor == MAGNETOMETER ? magnet_raw : accel_raw),
		 (sensor == MAGNETOMETER ? 6 : 120),
		 this, I2C_COMM_FINISHED);

	wait(I2C_COMM_FINISHED);

	if (sensor == MAGNETOMETER) {
		int16_t temp_raw = 0;
		i2c.read(addr[sensor], 0x2E | 0x80, (uint8_t *) &temp_raw, 2,
			 this, I2C_COMM_FINISHED);
		wait(I2C_COMM_FINISHED);
		temp = ((float) temp_raw) / 8 + 25;
	}

	return true;
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
