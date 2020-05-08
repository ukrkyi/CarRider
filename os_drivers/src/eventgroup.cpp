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

bool EventGroup::notifyISR(Event event)
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

void EventGroup::notify(Event event)
{
	if (xPortIsInsideInterrupt())
		notifyISR(event);
	else
		xEventGroupSetBits(handle, event);
}

Event EventGroup::wait(int event)
{
	EventBits_t uxBits = xEventGroupWaitBits(
		    handle,
		    event,
		    pdTRUE,
		    pdFALSE,
		    portMAX_DELAY );

	return Event(uxBits & event);
}

void EventGroup::clear(int event)
{
	xEventGroupClearBits(handle, event);
}
