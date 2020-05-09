/* (c) 2020 ukrkyi */
#include "i2c.h"

#include "stm32f4xx_ll_dma.h"

#include "system.h"
#include "eventgroup.h"

#define I2C_WRITE	0x0
#define I2C_READ	0x1

I2C::I2C(I2C_TypeDef * i2c, GPIO_TypeDef * port, uint16_t sclPin, uint16_t sdaPin, uint32_t dmaStream)
	: i2c(i2c), dmaStream(dmaStream), state(STANDBY), operation(NONE), mutex()
{
	if (port == GPIOB)
		__HAL_RCC_GPIOB_CLK_ENABLE();
	else
		while (1);

	GPIO_InitTypeDef initPin;
	initPin.Pin = sclPin | sdaPin;
	initPin.Mode = GPIO_MODE_AF_OD;
	initPin.Pull = GPIO_NOPULL;
	initPin.Speed = GPIO_SPEED_MEDIUM;
	if (i2c == I2C1)
		initPin.Alternate = GPIO_AF4_I2C1;
	else
		while (1);

	HAL_GPIO_Init(port, &initPin);

	__HAL_RCC_DMA1_CLK_ENABLE();

	// TX: DMA 2 Stream 7
	LL_DMA_ConfigTransfer(DMA1, dmaStream,
			      LL_DMA_DIRECTION_PERIPH_TO_MEMORY |
			      LL_DMA_MODE_NORMAL                |
			      LL_DMA_PERIPH_NOINCREMENT         |
			      LL_DMA_MEMORY_INCREMENT           |
			      LL_DMA_MDATAALIGN_BYTE            |
			      LL_DMA_PDATAALIGN_BYTE            |
			      LL_DMA_PRIORITY_VERYHIGH);
	if (i2c == I2C1)
		LL_DMA_SetChannelSelection(DMA1, dmaStream, LL_DMA_CHANNEL_1);
	else
		while (1);

	if (dmaStream == LL_DMA_STREAM_0) {
		NVIC_SetPriority(DMA1_Stream0_IRQn, 7); // Rx

		NVIC_EnableIRQ(DMA1_Stream0_IRQn);
	} else if (dmaStream == LL_DMA_STREAM_5) {
		NVIC_SetPriority(DMA1_Stream5_IRQn, 7); // Rx

		NVIC_EnableIRQ(DMA1_Stream5_IRQn);
	} else
		assert_param(0);

	if (i2c == I2C1)
		__HAL_RCC_I2C1_CLK_ENABLE();
	else
		while (1);

	LL_I2C_InitTypeDef i2cInit = {
		.PeripheralMode = LL_I2C_MODE_I2C,
		.ClockSpeed = 400000UL,
		.DutyCycle = LL_I2C_DUTYCYCLE_2,
		.AnalogFilter = LL_I2C_ANALOGFILTER_ENABLE,
		.DigitalFilter = 0x6,
		.OwnAddress1 = 0x0,
		.TypeAcknowledge = LL_I2C_ACK,
		.OwnAddrSize = LL_I2C_OWNADDRESS1_7BIT
	};

	LL_I2C_Init(I2C1, &i2cInit);

	// WARNING currently don't handle errors
	//NVIC_SetPriority(I2C1_ER_IRQn, I2C_IT_PRIORITY);
	NVIC_SetPriority(I2C1_EV_IRQn, I2C_IT_PRIORITY);
	NVIC_EnableIRQ(I2C1_EV_IRQn);

	mutex.give();
}

void I2C::write(uint8_t address, uint8_t reg, uint8_t *data, size_t size)
{
	mutex.take();

	commAddress = address;
	commRegister = reg;
	dataPtr = data;
	dataLeft = size;

	operation = WRITE;
	state = START;

	LL_I2C_EnableIT_TX(I2C1);

	LL_I2C_GenerateStartCondition(I2C1);
}

void I2C::read(uint8_t address, uint8_t reg, uint8_t *data, size_t size)
{
	mutex.take();

	commAddress = address;
	commRegister = reg;
	dataPtr = data;
	dataLeft = size;

	operation = READ;
	state = START;

	LL_DMA_SetDataLength(DMA1, dmaStream, dataLeft);
	LL_DMA_ConfigAddresses(DMA1, dmaStream, (uint32_t) &(i2c->DR), (uint32_t) dataPtr, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);

	LL_DMA_EnableIT_TC(DMA1, dmaStream);
	LL_DMA_EnableIT_TE(DMA1, dmaStream);

	LL_DMA_EnableStream(DMA1, dmaStream);

	LL_I2C_EnableIT_RX(I2C1);

	LL_I2C_GenerateStartCondition(I2C1);
}

I2C &I2C::getInstance()
{
	static I2C i2c(I2C1, GPIOB, GPIO_PIN_6, GPIO_PIN_7, LL_DMA_STREAM_0);
	return i2c;
}

void I2C::processInterrupt()
{
	if (LL_I2C_IsActiveFlag_SB(i2c)) {
		// Start condition transmitted
		if (state == START) {
			// Start address transmission
			state = SEND_ADDRESS;
			LL_I2C_TransmitData8(i2c, (commAddress << 1) | I2C_WRITE);
		} else {
			// Repeated start
			LL_I2C_TransmitData8(i2c, (commAddress << 1) | I2C_READ);
			if (dataLeft == 1)
				LL_I2C_AcknowledgeNextData(i2c, LL_I2C_NACK);
			// Enable DMA RX
			LL_I2C_EnableDMAReq_RX(i2c);
			return;
		}
	}

	if (LL_I2C_IsActiveFlag_ADDR(i2c)) {
		LL_I2C_ClearFlag_ADDR(i2c);
		if (state != SEND_ADDRESS) { // We're receiving using DMA
			LL_I2C_DisableIT_EVT(i2c);
			return;
		}
	}

	if (LL_I2C_IsActiveFlag_TXE(i2c)) {
		switch (state) {
		case SEND_ADDRESS:
			// We just sent address, need transmit register address
			state = SEND_REGISTER;
			LL_I2C_TransmitData8(i2c, commRegister);
			break;
		case SEND_REGISTER:
			// We sent register address
			state = PROCESS_DATA;
			if (operation == READ) {
				LL_I2C_GenerateStartCondition(i2c); // Repeated Start
				break;
			}
			/* fall through */
		case PROCESS_DATA:
			/** This check is necessary since TXE flag could be asserted
			 * prior to SB flag when Repeated Start is generated */
			if (operation == READ)
				break;
			// Write data
			if (dataLeft){
				dataLeft--;
				LL_I2C_TransmitData8(i2c, *(dataPtr--));
			} else {
				state = STANDBY;
				LL_I2C_GenerateStopCondition(i2c);
				LL_I2C_DisableIT_EVT(i2c);
				EventGroup::getInstance().notifyISR(I2C_COMM_FINISHED);
				mutex.giveISR();
			}
			break;
		default:
			while(1);
		}
	}
}

void I2C::processRxFinish()
{
	if (dmaStream == LL_DMA_STREAM_0) {
		if (LL_DMA_IsActiveFlag_TE0(DMA1)) {
			// Transmit Error
			LL_DMA_ClearFlag_TE0(DMA1);
			while (1);
		}

		if (LL_DMA_IsActiveFlag_TC0(DMA1)) {
			// Transmit completed
			LL_DMA_ClearFlag_TC0(DMA1);
		}
	} else {
		while (1);
	}

	LL_I2C_DisableDMAReq_RX(i2c);
	LL_I2C_GenerateStopCondition(i2c);

	state = STANDBY;
	EventGroup::getInstance().notifyISR(I2C_COMM_FINISHED);
	mutex.giveISR();
}
