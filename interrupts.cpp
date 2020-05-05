/* (c) 2020 ukrkyi */

#include "ultrasonic.h"
#include "uart.h"

extern "C" {

void TIM2_IRQHandler(void) {
	Ultrasonic::getInstance().processEcho(0);
}

void DMA2_Stream7_IRQHandler(void) {
	UART::getInstance().processTxCplt();
}

void DMA2_Stream2_IRQHandler(void) {
	UART::getInstance().processRxCplt();
}

void USART1_IRQHandler(void) {
	UART::getInstance().processRxCplt();
}

}
