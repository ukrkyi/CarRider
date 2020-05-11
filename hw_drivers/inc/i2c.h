/* (c) 2020 ukrkyi */
#ifndef I2C_H
#define I2C_H

#include "stm32f4xx_ll_i2c.h"

#include "semaphore.h"
#include "task.hpp"

class I2C
{
	I2C_TypeDef * i2c;
	uint32_t dmaStream;
	uint8_t commAddress, commRegister;
	const uint8_t * dataPtr;
	size_t dataLeft;
	enum State {
		STANDBY = 0x00,
		START,
		SEND_ADDRESS,
		SEND_REGISTER,
		PROCESS_DATA,
	} state;
	enum OperationType {
		NONE,
		WRITE,
		READ
	} operation;
	BinarySemaphore mutex;
	Task * notifyTask;
	uint32_t notifyBit;
	I2C(I2C_TypeDef *i2c, GPIO_TypeDef *port, uint16_t sclPin, uint16_t sdaPin, uint32_t dmaStream);
public:
	I2C() = delete;
	I2C(const I2C&) = delete;

	void write(uint8_t address, uint8_t reg, const uint8_t *data, size_t size, Task * task = NULL, uint32_t bit = 0);
	void read(uint8_t address, uint8_t reg, const uint8_t * data, size_t size, Task * task = NULL, uint32_t bit = 0);

	static I2C& getInstance();
	void processInterrupt();
	void processRxFinish();
};

#endif // I2C_H
