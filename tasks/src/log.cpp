/* (c) 2020 ukrkyi */
#include "log.h"

#include "wifi.h"

#include <cstdarg>

Log::Log(const char *name, UBaseType_t priority) :
	Task(name, this, priority), wifi(WiFi::getInstance()), evt(EventGroup::getInstance())
{}


void Log::run()
{
	WiFi::Data data;
	Event res;

	evt.clear(WIFI_COMMAND_ERROR);
	wifi.sendCommand(WiFi::POWER_ON);

	while (wifi.getState() == WiFi::WIFI_POWER_DOWN) {
		res = evt.wait(WIFI_COMMAND_ERROR | WIFI_STATE_CHANGED);

		if (res & WIFI_COMMAND_ERROR)
			while (1);         // TODO handle error
	}

	if (ap.permanent) {
		vTaskDelay(1000);

		if (wifi.getState() != WiFi::WIFI_CONNECTED) {
			evt.clear(WIFI_COMMAND_ERROR | WIFI_STATE_CHANGED);
			wifi.sendCommand(WiFi::WIFI_CONNECT, (void*)&ap);

			res = evt.wait(WIFI_COMMAND_ERROR | WIFI_STATE_CHANGED);

			if (res & WIFI_COMMAND_ERROR)
				while (1);         // TODO handle error
		}
	} else {
		evt.clear(WIFI_COMMAND_ERROR | WIFI_STATE_CHANGED);
		wifi.sendCommand(WiFi::WIFI_CONNECT, (void*)&ap);

		res = evt.wait(WIFI_COMMAND_ERROR | WIFI_STATE_CHANGED);

		if (res & WIFI_COMMAND_ERROR)
			while (1);         // TODO handle error
	}

	connectTcp();

	evt.clear(WIFI_CMD_PROCESSED);
	wifi.sendCommand(WiFi::TCP_SEND, &(data = { 3, "o/\n" }));
	evt.wait(WIFI_CMD_PROCESSED);

	evt.notify(LOG_TASK_READY);

	while(1) {
		wait();

		while (!buffer.empty()) {
			BufferType::chunk chunk = buffer.getContiniousData(2048);

			data.len = chunk.length();
			data.data = chunk;

			do {
				if (wifi.getState() != WiFi::TCP_CONNECTED)
					connectTcp();

				evt.clear(WIFI_COMMAND_ERROR | WIFI_CMD_PROCESSED);
				wifi.sendCommand(WiFi::TCP_SEND, &data);
				res = evt.wait(WIFI_CMD_PROCESSED);
			} while(res & WIFI_COMMAND_ERROR);

			mutex.lock();
			buffer.processed(chunk);
			mutex.unlock();
		}
	}
}

void Log::connectTcp()
{
	Event res;

	while (wifi.getState() != WiFi::TCP_CONNECTED) {
		while (wifi.getState() == WiFi::WIFI_DISCONNECTED)
			res = evt.wait(WIFI_STATE_CHANGED);

		wifi.sendCommand(WiFi::TCP_CONNECT, (void*)&comp);
		res = evt.wait(WIFI_COMMAND_ERROR | WIFI_STATE_CHANGED);

		if (res & WIFI_COMMAND_ERROR) {
			vTaskDelay(1000);
		}
	}
}

Log &Log::getInstance()
{
	static Log log("log", LOG_TASK_PRIORITY);
	return log;
}

bool Log::write(const char *format, ...)
{
	bool result;
	va_list args;
	va_start(args, format);
	mutex.lock();
	result = buffer.write(format, args);
	mutex.unlock();
	va_end(args);
	this->notify();
	return result;
}
