/* (c) 2020 ukrkyi */
#ifndef WIFI_H
#define WIFI_H

#include "task.hpp"

#include "queue.hpp"
#include "buffer.h"
#include "mutex.h"
#include "eventgroup.h"

#include <uart.h>

#define UART_RX_QUEUE_LEN	5
#define COMMAND_QUEUE_LEN	10

class WiFi : public Task
{
public:
	enum Command {
		POWER_ON,
		WIFI_CONNECT,
		ESP_UPDATE,
		TCP_CONNECT,
		TCP_SEND,
		COMMAND_NUM
	};

	enum Response {
		GENERIC_OK,
		GENERIC_ERROR,
		GENERIC_FAIL,
		GENERIC_BUSY,
		WIFI_POWERED_ON,
		WIFI_CONNECTION_ACQUIRED,
		WIFI_CONNECTION_LOST,
		DATA_RECEIVED,
		UPDATE_PROGRESS,
		WIFI_CONNECTION_INFO,
		TCP_SEND_OK,
		TCP_SEND_FAIL,
	};

	enum State {
		WIFI_POWER_DOWN,
		WIFI_DISCONNECTED,
		WIFI_CONNECTED,
		TCP_CONNECTED,
	};

	struct Socket {
		const char * ip;
		unsigned port;
	};

	struct AccessPoint {
		const char * ssid;
		const char * pass;
		bool permanent;
	};

	struct Data {
		size_t len;
		union {
			const char * const_str;
			char * string;
			uint8_t * data;
		};
	};

	static WiFi& getInstance();

	/** Send command to WiFi task */
	void sendCommand(Command cmd, void * data = NULL);

	/** Process command with blocking current task */
	bool processCommand(Command cmd, void * data = NULL);

	size_t getPendingDataLength();
	inline State getState() const {return state;}

private:
	typedef InputBuffer<50> BufferType;

	struct CommandInfo {
		Command cmd;
		void * data;
	};

	uint8_t uartQueueBuffer[sizeof (size_t) * UART_RX_QUEUE_LEN];
	uint8_t commandQueueBuffer[sizeof (CommandInfo) * COMMAND_QUEUE_LEN];
	Queue<size_t> uartQueue;
	Queue<CommandInfo> commandQueue;
	BufferType rxBuffer;
	UART & uart;
	GPIO_TypeDef * enPort;
	uint32_t enPin;
	Mutex processInProgress;
	State state;
	size_t pendingLength;
	uint8_t messageBuffer[20];
	EventGroup & evt;
	WiFi(const char * name, UBaseType_t priority, UART& uart, GPIO_TypeDef * portEnable, uint32_t pinEnable);
	void run();

	inline void setState(const State &value) {
		state = value;
		evt.notify(WIFI_STATE_CHANGED);
	}

	static bool (WiFi::* const handler[])(void *);

	int processResponse(bool wait = false, uint8_t *parameter = NULL);

	void processMessage(Response msg, uint8_t * data);

	bool powerOn(void *);
	bool wifiConnect(void * data);
	bool tcpConnectCmd(void * data);
	bool tcpSendCmd(void * data);
	bool updateCmd(void*);
	bool changeBaud(void * data);

	void sendMessage(int cmd, ...);
	Response getResponse(int cmd, uint8_t *parameter = NULL);

	int argToInt(uint8_t * data) const;
};

#endif // WIFI_H
