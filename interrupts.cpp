/* (c) 2020 ukrkyi */

#include "ultrasonic.h"
#include "uart.h"
#include "i2c.h"

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

void I2C1_EV_IRQHandler(void) {
	I2C::getInstance().processInterrupt();
}

void DMA1_Stream0_IRQHandler(void) {
	I2C::getInstance().processRxFinish();
}

}
