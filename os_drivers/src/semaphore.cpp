/* (c) 2020 ukrkyi */
#include "semaphore.h"

BinarySemaphore::BinarySemaphore()
{
	handle = xSemaphoreCreateBinaryStatic(&storage);
}

Semaphore::~Semaphore()
{
	vSemaphoreDelete(handle);
}

void Semaphore::take()
{
	xSemaphoreTake(handle, portMAX_DELAY);
}

void Semaphore::give()
{
	xSemaphoreGive(handle);
}

bool Semaphore::giveISR()
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE, xResult;

	xResult = xSemaphoreGiveFromISR( handle, &xHigherPriorityTaskWoken );

	if ( xResult != pdFAIL ){
		portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
		return true;
	} else {
		return false;
	}
}
