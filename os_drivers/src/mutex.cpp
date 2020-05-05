/* (c) 2020 ukrkyi */
#include "mutex.h"

Mutex::Mutex()
{
	handle = xSemaphoreCreateMutexStatic(&storage);
}

Mutex::~Mutex()
{
	vSemaphoreDelete(handle);
}

void Mutex::lock()
{
	xSemaphoreTake(handle, portMAX_DELAY);
}

void Mutex::unlock()
{
	xSemaphoreGive(handle);
}
