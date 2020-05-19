/* (c) 2020 ukrkyi */
#ifndef LOG_H
#define LOG_H

#include "task.hpp"
#include "buffer.h"
#include "mutex.h"
#include "wifi.h"

class Log : public Task
{
	using BufferType = OutputBuffer<10240>;
	WiFi& wifi;
	EventGroup &evt;
	BufferType buffer;
	Mutex mutex;

	const WiFi::AccessPoint ap = { "ukrkyi-hotspot", "qwe123QWE!@#", false };
	const WiFi::Socket comp = { "10.42.0.1", 1488 };

	Log(const char *name, UBaseType_t priority);
	void run();

	void connectTcp();
public:
	static Log& getInstance();

	bool write(const char *format, ...);
};

#endif // LOG_H
