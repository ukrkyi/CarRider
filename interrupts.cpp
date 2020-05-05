/* (c) 2020 ukrkyi */

#include "ultrasonic.h"

extern "C" {

void TIM2_IRQHandler(void) {
	Ultrasonic::getInstance().processEcho(0);
}

}
