/* (c) 2020 ukrkyi */
#include "uart.h"

#include "stm32f4xx_ll_dma.h"
#include "stm32f4xx_ll_usart.h"

#include "task.hpp"

UART::UART(GPIO_TypeDef *txPort, uint16_t txPin, GPIO_TypeDef *rxPort, uint16_t rxPin,
	   USART_TypeDef *uart, uint32_t baudrate,
	   DMA_TypeDef *dma, uint32_t txStream, uint32_t rxStream) :
	uart(uart), dma(dma), streamTx(txStream), streamRx(rxStream), queue(NULL), notifyReception(NULL)
{
	assert_param((txPort == GPIOA || txPort == GPIOB) && (rxPort == GPIOA || rxPort == GPIOB));

	if (txPort == GPIOA || rxPort == GPIOA)
		__HAL_RCC_GPIOA_CLK_ENABLE();
	else if (txPort == GPIOB || rxPort == GPIOB)
		__HAL_RCC_GPIOB_CLK_ENABLE();

	GPIO_InitTypeDef gpio;

	gpio.Pin = txPin;
	gpio.Mode = GPIO_MODE_AF_PP;
	gpio.Pull = GPIO_PULLUP;
	gpio.Speed = GPIO_SPEED_FREQ_MEDIUM;

	if (uart == USART1)
		gpio.Alternate = GPIO_AF7_USART1;
	else
		assert_param(0);

	if (txPort == rxPort)
		gpio.Pin |= rxPin;
	else {
		HAL_GPIO_Init(txPort, &gpio);
		gpio.Pin = rxPin;
	}

	HAL_GPIO_Init(rxPort, &gpio);

	if (dma == DMA2)
		__HAL_RCC_DMA2_CLK_ENABLE();
	else
		assert_param(0);

	// TX: DMA 2 Stream 7
	LL_DMA_ConfigTransfer(dma, txStream,
			      LL_DMA_DIRECTION_MEMORY_TO_PERIPH |
			      LL_DMA_MODE_NORMAL                |
			      LL_DMA_PERIPH_NOINCREMENT         |
			      LL_DMA_MEMORY_INCREMENT           |
			      LL_DMA_MDATAALIGN_BYTE            |
			      LL_DMA_PDATAALIGN_BYTE            |
			      LL_DMA_PRIORITY_MEDIUM);

	if (dma == DMA2 && uart == USART1)
		LL_DMA_SetChannelSelection(dma, txStream, LL_DMA_CHANNEL_4);
	else
		assert_param(0);

	// RX: DMA 2 Stream 2
	LL_DMA_ConfigTransfer(dma, rxStream,
			      LL_DMA_DIRECTION_PERIPH_TO_MEMORY |
			      LL_DMA_MODE_CIRCULAR              |
			      LL_DMA_PERIPH_NOINCREMENT         |
			      LL_DMA_MEMORY_INCREMENT           |
			      LL_DMA_MDATAALIGN_BYTE            |
			      LL_DMA_PDATAALIGN_BYTE            |
			      LL_DMA_PRIORITY_HIGH);

	if (dma == DMA2 && uart == USART1)
		LL_DMA_SetChannelSelection(dma, rxStream, LL_DMA_CHANNEL_4);
	else
		assert_param(0);

	if (dma == DMA2 && txStream == LL_DMA_STREAM_7 && rxStream == LL_DMA_STREAM_2) {
		NVIC_SetPriority(DMA2_Stream7_IRQn, 10); // Tx
		NVIC_SetPriority(DMA2_Stream2_IRQn, 5); // Rx

		NVIC_EnableIRQ(DMA2_Stream7_IRQn);
		NVIC_EnableIRQ(DMA2_Stream2_IRQn);
	} else
		assert_param(0);

	if (uart == USART1)
		__HAL_RCC_USART1_CLK_ENABLE();
	else
		assert_param(0);

	LL_USART_InitTypeDef uartInit;

	uartInit.BaudRate = baudrate;
	uartInit.DataWidth = LL_USART_DATAWIDTH_8B;
	uartInit.StopBits = LL_USART_STOPBITS_1;
	uartInit.Parity = LL_USART_PARITY_NONE;
	uartInit.TransferDirection = LL_USART_DIRECTION_TX_RX;
	uartInit.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
	uartInit.OverSampling = LL_USART_OVERSAMPLING_16;

	LL_USART_Init(uart, &uartInit);
	LL_USART_ConfigAsyncMode(uart);

	if (uart == USART1) {
		NVIC_SetPriority(USART1_IRQn, 5);
		NVIC_EnableIRQ(USART1_IRQn);
	} else
		assert_param(0);

	LL_USART_Enable(uart);

	mutex.give();
}

UART &UART::getInstance()
{
	static UART uart(GPIOA, GPIO_PIN_9, GPIOA, GPIO_PIN_10,
		  USART1, 115200, DMA2, LL_DMA_STREAM_7, LL_DMA_STREAM_2);
	return uart;
}

void UART::send(const uint8_t *data, uint32_t length)
{
	// Lock a mutex so no other task can write to UART
	mutex.take();

	LL_DMA_SetDataLength(dma, streamTx, length);
	LL_DMA_ConfigAddresses(dma, streamTx, (uint32_t) data, (uint32_t) &(uart->DR), LL_DMA_DIRECTION_MEMORY_TO_PERIPH);

	LL_DMA_EnableIT_TC(dma, streamTx);
	LL_DMA_EnableIT_TE(dma, streamTx);

	LL_DMA_EnableStream(dma, streamTx);

	LL_USART_ClearFlag_TC(uart);

	LL_USART_EnableDMAReq_TX(uart);
}


void UART::startRx(uint8_t *buffer, uint32_t size, Queue<size_t> &queueRef, Task * notify)
{
	queue = &queueRef;
	notifyReception = notify;
	bufLength = size;

	LL_DMA_SetMode(dma, streamRx, LL_DMA_MODE_CIRCULAR);

	LL_DMA_SetDataLength(dma, streamRx, size);
	LL_DMA_ConfigAddresses(dma, streamRx, (uint32_t) &(uart->DR), (uint32_t) buffer, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);

	LL_DMA_EnableIT_TC(dma, streamRx);
	LL_DMA_EnableIT_HT(dma, streamRx);
	LL_DMA_EnableIT_TE(dma, streamRx);

	LL_DMA_EnableStream(dma, streamRx);

	LL_USART_ClearFlag_ORE(uart);

	// LL_USART_EnableIT_PE(uart); // We don't need this since parity check not enabled
//	LL_USART_EnableIT_ERROR(uart); // TODO Should we handle this?
	LL_USART_EnableIT_IDLE(uart);

	LL_USART_EnableDMAReq_RX(uart);
}

void UART::processTxCplt()
{
	if (streamTx == LL_DMA_STREAM_7) {
		if (LL_DMA_IsActiveFlag_TE7(dma)) {
			// Transmit Error
			LL_DMA_ClearFlag_TE7(dma);
			while (1);
		}

		if (LL_DMA_IsActiveFlag_TC7(dma)) {
			// Transmit completed
			LL_DMA_ClearFlag_TC7(dma);

			LL_DMA_DisableIT_TC(dma, streamTx);
			LL_DMA_DisableIT_TE(dma, streamTx);

			LL_USART_DisableDMAReq_TX(uart);

			mutex.giveISR();
		}
	} else {
		while(1);
	}
}

void UART::processRxCplt()
{
	unsigned position;

	if (streamRx == LL_DMA_STREAM_2) {
		if (LL_DMA_IsActiveFlag_TE2(dma)) {
			// Transmit Error
			LL_DMA_ClearFlag_TE2(dma);
			while (1);
		}

		if (LL_DMA_IsActiveFlag_TC2(dma)) {
			// Transmit completed
			LL_DMA_ClearFlag_TC2(dma);
		}

		if (LL_DMA_IsActiveFlag_HT2(dma)) {
			// Half transmit completed
			LL_DMA_ClearFlag_HT2(dma);
		}
	} else {
		while (1);
	}

//	if (LL_USART_IsActiveFlag_FE(uart)) {
//		// Framing error
//		LL_USART_ClearFlag_FE(uart);
//		// while(1);
//		// Don't stop, since error will be handled by high-level tasks and we can do nothing
//		// TODO report error
//		error = true;
//	}

//	if (LL_USART_IsActiveFlag_NE(uart)) {
//		// Noise error
//		LL_USART_ClearFlag_NE(uart);
//		// while(1);
//		// Don't stop, since error will be handled by high-level tasks and we can do nothing
//		// TODO report error
//		error = true;
//	}

	if (LL_USART_IsActiveFlag_IDLE(uart)) {
		// Transmission end - IDLE frame detected
		LL_USART_ClearFlag_IDLE(uart);
	}

	position = bufLength - LL_DMA_GetDataLength(dma, streamRx);

	queue->putISR(position);
	if (notifyReception != NULL) {
		 notifyReception->notifyISR();
	}
}
