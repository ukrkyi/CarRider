/* (c) 2020 ukrkyi */
#ifndef SEMAPHORE_HPP
#define SEMAPHORE_HPP

#include <FreeRTOS.h>
#include "semphr.h"

class Semaphore
{
protected:
	StaticSemaphore_t storage;
	SemaphoreHandle_t handle;
	Semaphore() {}
public:
	~Semaphore();
	void take();
	void give();
	bool giveISR();
};

class BinarySemaphore : public Semaphore
{
public:
	BinarySemaphore();
};

#endif // SEMAPHORE_HPP
