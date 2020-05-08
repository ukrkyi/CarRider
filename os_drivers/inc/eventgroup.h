/* (c) 2020 ukrkyi */
#ifndef EVENTGROUP_H
#define EVENTGROUP_H

#include "FreeRTOS.h"
#include "event_groups.h"

enum Event {
	ULTRASONIC_MEASUREMENT_COMPLETED = 0x1,
	ESP_REPLY_RECEIVED = 0x2,
};

class EventGroup
{
	StaticEventGroup_t storage;
	EventGroupHandle_t handle;
	EventGroup();
	~EventGroup();
public:
	static EventGroup& getInstance();
	bool notifyISR(Event event);
	void notify(Event event);
	/** event should be a combination from Event enum */
	Event wait(int event);
	void clear(int event);
};

#endif // EVENTGROUP_H
