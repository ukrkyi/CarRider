/* (c) 2020 ukrkyi */
#ifndef EVENTGROUP_H
#define EVENTGROUP_H

#include "FreeRTOS.h"
#include "event_groups.h"

enum Event {
	NO_EVENT             = 0x00,
	// Sensors go here
	ULTRASONIC_NEW_DATA  = 0x01,
	// WiFi go here
	WIFI_STATE_CHANGED   = 0x10,
	WIFI_DATA_RECEIVED   = 0x20,
	WIFI_CMD_PROCESSED   = 0x40,
	WIFI_COMMAND_ERROR   = 0x80,
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
