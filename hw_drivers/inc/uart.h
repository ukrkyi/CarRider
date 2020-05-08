/* (c) 2020 ukrkyi */
#ifndef UART_H
#define UART_H

#include <stm32f4xx.h>

#include "semaphore.h"

//#include "buffer.h"

#include "queue.hpp"

class Task;

#define DATA_OVERWRITE	0xFFFF0000UL

class UART
{
	USART_TypeDef *uart;
	DMA_TypeDef *dma;
	uint32_t streamTx, streamRx;
	BinarySemaphore mutex;
	Queue<size_t> * queue;
	Task * notifyReception;
	size_t bufLength;
	UART(GPIO_TypeDef * txPort, uint16_t txPin,
	     GPIO_TypeDef * rxPort, uint16_t rxPin,
	     USART_TypeDef * uart, uint32_t baudrate,
	     DMA_TypeDef *dma, uint32_t txStream, uint32_t rxStream);
public:
	UART() = delete;
	UART(const UART&) = delete;
	static UART& getInstance();

	void send(const uint8_t *data, uint32_t length);
	//template<int N> void send(typename Buffer<N>::chunk& data);
	void startRx(uint8_t * buffer, uint32_t size, Queue<size_t>& queueRef, Task * notify = NULL);
	void stopRx();
	void receive(uint8_t * buffer, uint32_t size, void (*callback)());
	//template<int N> void startRx(Buffer<N>& buffer, QueueHandle_t queueHandle) {startRx(buffer, N, queueHandle);}
	void processTxCplt(void);
	void processRxCplt(void);
};

//template<int N>
//void UART::send(typename Buffer<N>::chunk &data)
//{
//	if (!data.isOverlap())
//		send(data, data.length());
//	else {
//		typename Buffer<N>::chunk part1 = data.getFirstContPart(), part2 = data.getSecondContPart();
//		send(part1, part2.length());
//		send(part2, part2.length());
//	}
//}

#endif // UART_H
