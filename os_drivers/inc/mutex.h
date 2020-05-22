/* (c) 2020 ukrkyi */
#ifndef MUTEX_H
#define MUTEX_H

#include <FreeRTOS.h>
#include "semphr.h"

class Mutex
{
	StaticSemaphore_t storage;
	SemaphoreHandle_t handle;
public:
	Mutex();
	~Mutex();
	void lock();
	void unlock();
	bool isAlreadyLocked();
};

#endif // MUTEX_H
