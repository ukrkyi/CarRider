/* (c) 2020 ukrkyi */
#include "eventgroup.h"

EventGroup::EventGroup()
{
	handle = xEventGroupCreateStatic(&storage);
}

EventGroup::~EventGroup()
{
	vEventGroupDelete(handle);
}

EventGroup &EventGroup::getInstance()
{
	static EventGroup sensorEvents;

	return sensorEvents;
}

bool EventGroup::notify(Event event)
{
	BaseType_t xHigherPriorityTaskWoken, xResult;

	xHigherPriorityTaskWoken = pdFALSE;

	xResult = xEventGroupSetBitsFromISR(handle, event, &xHigherPriorityTaskWoken);

	if ( xResult != pdFAIL ){
		portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
		return true;
	} else {
		return false;
	}
}

bool EventGroup::wait(Event event)
{
	EventBits_t uxBits = xEventGroupWaitBits(
		    handle,
		    event,
		    pdTRUE,
		    pdTRUE,
		    portMAX_DELAY );

	return (uxBits & event) == event;
}
