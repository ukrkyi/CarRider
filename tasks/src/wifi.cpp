/* (c) 2020 ukrkyi */
#include "wifi.h"

#include <cstdio>
#include <cstring>
#include <cstdarg>

#include <algorithm>

enum AtCommand {
	AT,
	AT_GET_VERSION,
	AT_UART_CONFIG,
	AT_WIFI_MODE,
	AT_WIFI_CONNECT,
	AT_WIFI_CONNECT_SAVE,
	AT_WIFI_QUERY,
	AT_WIFI_POWER,
	AT_TCP_CONNECT,
	AT_TCP_SEND,
	AT_RECEIVE_MODE,
	AT_UPDATE,
	AT_RESTORE_FACTORY,
	AT_WRITE_DATA,
	AT_COMMAND_NUM
};

struct CommandType {
	const char * name;
	WiFi::Response ok, error;
};

static const char * wifi_evt[] = {
	[WiFi::GENERIC_OK] = "OK",
	[WiFi::GENERIC_ERROR] = "ERROR",
	[WiFi::GENERIC_FAIL] = "FAIL",
	[WiFi::GENERIC_BUSY] = "busy p...",
	[WiFi::WIFI_POWERED_ON] = "ready",
	[WiFi::WIFI_CONNECTION_ACQUIRED] = "WIFI GOT IP",
	[WiFi::WIFI_CONNECTION_LOST] = "WIFI DISCONNECT",
	[WiFi::DATA_RECEIVED] = "+IPD,",
	[WiFi::UPDATE_PROGRESS] = "+CIPUPDATE:",
	[WiFi::WIFI_CONNECTION_INFO] = "+CWJAP:",
	[WiFi::TCP_SEND_OK] = "SEND OK",
	[WiFi::TCP_SEND_FAIL] = "SEND FAIL",
	[WiFi::TCP_CONNECTION_CLOSED] = "CLOSED",
};

static const int wifi_evt_num = sizeof(wifi_evt) / sizeof(char * );

static const CommandType command[] = {
	[AT] =			 {
		.name	= "AT",
		.ok	= WiFi::GENERIC_OK,
		.error	= WiFi::GENERIC_ERROR,
	},
	[AT_GET_VERSION] =	 {
		.name	= "AT+GMR",
		.ok	= WiFi::GENERIC_OK,
		.error	= WiFi::GENERIC_ERROR,
	},
	[AT_UART_CONFIG] =	 {
		.name	= "AT+UART_DEF=%d,8,1,0,0",
		.ok	= WiFi::GENERIC_OK,
		.error	= WiFi::GENERIC_ERROR,
	},
	[AT_WIFI_MODE] =	 {
		.name	= "AT+CWMODE_DEF=%d",
		.ok	= WiFi::GENERIC_OK,
		.error	= WiFi::GENERIC_ERROR,
	},
	[AT_WIFI_CONNECT] =	 {
		.name	= "AT+CWJAP_CUR=\"%s\",\"%s\"",
		.ok	= WiFi::GENERIC_OK,
		.error	= WiFi::GENERIC_FAIL,
	},
	[AT_WIFI_CONNECT_SAVE] = {
		.name	= "AT+CWJAP_DEF=\"%s\",\"%s\"",
		.ok	= WiFi::GENERIC_OK,
		.error	= WiFi::GENERIC_FAIL,
	},
	[AT_WIFI_QUERY] =	 {
		.name	= "AT+CWJAP?",
		.ok	= WiFi::WIFI_CONNECTION_INFO,
		.error	= WiFi::GENERIC_ERROR,
	},
	[AT_WIFI_POWER] =	{
		.name	= "AT+RFPOWER=%d",
		.ok	= WiFi::GENERIC_OK,
		.error	= WiFi::GENERIC_FAIL,
	},
	[AT_TCP_CONNECT] =	 {
		.name	= "AT+CIPSTART=\"TCP\",\"%s\",%d",
		.ok	= WiFi::GENERIC_OK,
		.error	= WiFi::GENERIC_ERROR,
	},
	[AT_TCP_SEND] =		 {
		.name	= "AT+CIPSEND=%d",
		.ok	= WiFi::GENERIC_OK,
		.error	= WiFi::GENERIC_ERROR,
	},
	[AT_RECEIVE_MODE] =	 {
		.name	= "AT+CIPRECVMODE=%d",
		.ok	= WiFi::GENERIC_OK,
		.error	= WiFi::GENERIC_ERROR,
	},
	[AT_UPDATE] =		 {
		.name	= "AT+CIUPDATE",
		.ok	= WiFi::UPDATE_PROGRESS,
		.error	= WiFi::GENERIC_ERROR,
	},
	[AT_RESTORE_FACTORY] =	 {
		.name	= "AT+RESTORE",
		.ok	= WiFi::GENERIC_OK,
		.error	= WiFi::GENERIC_ERROR,
	},
	[AT_WRITE_DATA] =	 {
		.name	= NULL,
		.ok	= WiFi::TCP_SEND_OK,
		.error	= WiFi::TCP_SEND_FAIL,
	},
};

bool (WiFi::* const (WiFi::handler)[])(void *) = {
	[POWER_ON] = &WiFi::powerOn,
	[WIFI_CONNECT] = &WiFi::wifiConnect,
	[ESP_UPDATE] = &WiFi::updateCmd,
	[TCP_CONNECT] = &WiFi::tcpConnectCmd,
	[TCP_SEND] = &WiFi::tcpSendCmd,
};

WiFi::WiFi(const char *name, UBaseType_t priority, UART& uart, GPIO_TypeDef * portEnable, uint32_t pinEnable) :
	Task(name, this, priority),
	uartQueue(uartQueueBuffer, UART_RX_QUEUE_LEN), commandQueue(commandQueueBuffer, COMMAND_QUEUE_LEN),
	uart(uart), enPort(portEnable), enPin(pinEnable), state(WIFI_POWER_DOWN), pendingLength(0),
	evt(EventGroup::getInstance())
{
	if (enPort == GPIOA)
		__HAL_RCC_GPIOA_CLK_ENABLE();
	else
		while (1);

	HAL_GPIO_WritePin(enPort, enPin, GPIO_PIN_RESET);

	GPIO_InitTypeDef GPIO_InitStruct;

	GPIO_InitStruct.Pin = enPin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(enPort, &GPIO_InitStruct);
}

void WiFi::run()
{
	while (1) {
		wait();

		do {
			while (!uartQueue.empty()) {
				// uart data available
				processInProgress.lock(); // prevent new commands from being processed
				do {
					int message = processResponse(false, messageBuffer);
					if (message != -1)
						processMessage(Response(message), messageBuffer);
				} while (rxBuffer.length());
				processInProgress.unlock();
			}

			if (!commandQueue.empty()) {
				// command received
				bool valid;
				CommandInfo cmd = commandQueue.take(valid);
				if (!valid)
					continue;

				evt.clear(WIFI_CMD_PROCESSED);
				if (!processCommand(cmd.cmd, cmd.data))
					evt.notify(WIFI_COMMAND_ERROR);
				if (commandQueue.empty()) // we processed all scheduled commands
					evt.notify(WIFI_CMD_PROCESSED);

			}
		} while (!commandQueue.empty());

	}
}

int WiFi::processResponse(bool wait, uint8_t * parameter)
{
	size_t pos;
	int match = -1;
	bool fullMatch = false;
	bool justFindEnd = false;
	size_t countMatch = 0;
	int i;

	while (!fullMatch || justFindEnd) {
		if (!rxBuffer.length()) {
			bool gotData = false;
			if (match != -1 || justFindEnd) {
				// We now have partial match, so we definitely need completion
				// WARNING we can stuck here if transmission error occurs :(
				// TODO add error handling to UART so we would know about it
				pos = uartQueue.take(gotData);
			} else          // We can stuck here if expected message not received
				pos = uartQueue.take(gotData, wait);
			if (!gotData)   // we didn't get data, match wasn't found - so we just return
				return -1;
		}

		BufferType::chunk data = rxBuffer.length()
					 ? rxBuffer.getData()
					 : rxBuffer.newData(pos);

		if (justFindEnd) {
			if (parameter != NULL) {
				BufferType::iterator begin = data.start();
				parameter = std::copy(begin, begin + data.index('\n'), parameter);
			}
			if (data.index('\n') < data.length()) {
				rxBuffer.next(data, data.index('\n') + 1);
				break;
			} else {
				rxBuffer.processed(data);
				continue;
			}
		}

		while (data.length() != 0 && !fullMatch) {
search:                 for (i = 0; (i < wifi_evt_num) && (!fullMatch); i++) {
				if ((match == -1)
				    || (match != -1
					&& (match == i
					    || strncmp(wifi_evt[match], wifi_evt[i], countMatch) == 0))) {
					if (data.beginsWith(wifi_evt[i] + countMatch)) {
						match = i;
						fullMatch = true;
					}  else if (data.beginOf(wifi_evt[i] + countMatch)) {
						match = i;
						fullMatch = false;
					}
				}
			}

			if (match != -1 && fullMatch == false) {
				if (!data.beginOf(wifi_evt[match] + countMatch)) {
					// No update was received through search, discard match
					match = -1;
					countMatch = 0;
					// Re-search all the posibilities since it can be new transaction
					goto search;
				} else
					// Indeed, we are propagating inside partial match
					countMatch += data.length();
			}

			if (fullMatch && parameter != NULL) {
				BufferType::iterator begin = data.start();
				parameter = std::copy(begin + (strlen(wifi_evt[match]) - countMatch),
						      begin + data.index('\n'), parameter);
			}

			if (data.index('\n') < data.length()) {
				// We can (and indeed should) discard any PARTIAL match here bc of moving to new line
				if (!fullMatch) {
					match = -1;
					countMatch = 0;
				}
				rxBuffer.next(data, data.index('\n') + 1);
			} else {
				if (fullMatch)
					justFindEnd = true;
				rxBuffer.processed(data);
			}
		}
	}

	return match;
}

void WiFi::processMessage(WiFi::Response msg, uint8_t *data)
{
	switch (msg) {
	case WIFI_POWERED_ON:
		setState(WIFI_DISCONNECTED);
		break;
	case WIFI_CONNECTION_ACQUIRED:
		setState(WIFI_CONNECTED);
		break;
	case WIFI_CONNECTION_LOST:
		setState(WIFI_DISCONNECTED);
		break;
	case TCP_CONNECTION_CLOSED:
		setState(WIFI_CONNECTED);
		break;
	case DATA_RECEIVED:
	{
		pendingLength += argToInt(data);
		evt.notify(WIFI_DATA_RECEIVED);
	}
	break;
	case GENERIC_OK:
	case GENERIC_ERROR:
	case GENERIC_FAIL:
	case TCP_SEND_OK:
	case UPDATE_PROGRESS:
	case WIFI_CONNECTION_INFO:
		break; // this will do nothing
	}
}

bool WiFi::powerOn(void *)
{
	HAL_GPIO_WritePin(enPort, enPin, GPIO_PIN_SET);

	uart.startRx(rxBuffer, rxBuffer.size(), uartQueue, this);

	return true;
}

bool WiFi::wifiConnect(void *data)
{
	if (state == WIFI_POWER_DOWN)
		return false;

	sendMessage(AT_WIFI_MODE, 1);
	Response resp = getResponse(AT_WIFI_MODE);

	if (resp != command[AT_WIFI_CONNECT].ok)
		return false;

	if (data == NULL) {
		uint8_t buffer[50];
		sendMessage(AT_WIFI_QUERY);

		resp = getResponse(AT_WIFI_QUERY, buffer);
		if (resp == command[AT_WIFI_QUERY].ok)
			return true;
		else
			return false;
	} else {
		AccessPoint *ap = (AccessPoint*)data;

		if (ap->permanent)
			sendMessage(AT_WIFI_CONNECT_SAVE, ap->ssid, ap->pass);
		else
			sendMessage(AT_WIFI_CONNECT, ap->ssid, ap->pass);

		resp = getResponse(AT_WIFI_CONNECT);

		if (resp == command[AT_WIFI_CONNECT].ok)
			return true;
		else
			return false;
	}
}

bool WiFi::tcpConnectCmd(void * data)
{
	if (state < WIFI_CONNECTED)
		return false;

	Socket * target = (Socket*)data;

	sendMessage(AT_RECEIVE_MODE, 1);

	Response resp = getResponse(AT_RECEIVE_MODE);

	if (resp != command[AT_RECEIVE_MODE].ok)
		return false;

	sendMessage(AT_TCP_CONNECT, target->ip, target->port);

	resp = getResponse(AT_TCP_CONNECT);

	if (resp == command[AT_TCP_CONNECT].ok) {
		setState(TCP_CONNECTED);
		return true;
	} else
		return false;
}

bool WiFi::tcpSendCmd(void *data)
{
	if (state != TCP_CONNECTED)
		return false;

	Data * tosend = (Data*)data;

	sendMessage(AT_TCP_SEND, tosend->len);

	Response resp = getResponse(AT_TCP_SEND);

	if (resp != command[AT_TCP_SEND].ok)
		return false;

	uart.send(tosend->data, tosend->len);

	resp = getResponse(AT_WRITE_DATA);
	return resp == command[AT_WRITE_DATA].ok;
}

bool WiFi::updateCmd(void *)
{
	if (state < WIFI_CONNECTED)
		return false;

	sendMessage(AT_UPDATE);

	Response resp = getResponse(AT_UPDATE, messageBuffer);// found server

	if (resp != command[AT_UPDATE].ok)
		return false;

	if (argToInt(messageBuffer) != 1)
		return false;

	resp = getResponse(AT_UPDATE, messageBuffer); // connected to server

	if (resp != command[AT_UPDATE].ok)
		return false;

	if (argToInt(messageBuffer) != 2)
		return false;

	resp = getResponse(AT_UPDATE, messageBuffer); // get version

	if (resp != command[AT_UPDATE].ok)
		return false;

	if (argToInt(messageBuffer) != 3)
		return false;

	do {
		resp = getResponse(AT_UPDATE, messageBuffer); // started update

		if (resp != command[AT_UPDATE].ok)
			return false;

	} while (argToInt(messageBuffer) != 4);

	setState(WIFI_DISCONNECTED);

	int result;
	do {
		result = processResponse(true, messageBuffer);
		if (result != -1)
			processMessage(Response(result), messageBuffer);
	} while (result != WIFI_POWERED_ON);

	sendMessage(AT_RESTORE_FACTORY);

	resp = getResponse(AT_RESTORE_FACTORY);

	if (resp != command[AT_RESTORE_FACTORY].ok)
		return false;

	setState(WIFI_DISCONNECTED);

	return true;
}

bool WiFi::changeBaud(void *data)
{
	uint32_t baud = *(uint32_t *) data;
	sendMessage(AT_UART_CONFIG, baud);
	Response resp = getResponse(AT_WIFI_MODE);

	if (resp == command[AT_WIFI_CONNECT].ok){
		uart.setBaud(baud);
		return true;
	} else
		return false;
}

void WiFi::sendMessage(int cmd, ...)
{
	static char commandBuffer[50];
	static size_t size;

	assert_param(cmd < AT_COMMAND_NUM);
	assert_param(command[cmd].name != NULL);

	va_list args;
	va_start(args, cmd);
	size = vsnprintf(commandBuffer, 50, command[cmd].name, args);
	va_end(args);

	assert_param(size < 50);
	uart.send((uint8_t*)commandBuffer, size);
	uart.send((uint8_t*)"\r\n", 2);
}

WiFi::Response WiFi::getResponse(int cmd, uint8_t * parameter)
{
	do {
		int result = processResponse(true, parameter ? parameter : messageBuffer);
		if (result == command[cmd].ok ||
		    result == command[cmd].error)
			return Response(result);
		else if (result != -1)
			processMessage(Response(result), parameter ? parameter : messageBuffer);
	} while (1);
}

int WiFi::argToInt(uint8_t *data) const
{
	char * end;
	int result = strtol((char*)data, &end, 10);

	if (*end != '\r')
		while (1);
	return result;
}

WiFi &WiFi::getInstance()
{
	static WiFi wifi("wifi", WIFI_TASK_PRIORITY, UART::getInstance(), GPIOA, GPIO_PIN_7);

	return wifi;
}

void WiFi::sendCommand(WiFi::Command cmd, void *data)
{
	CommandInfo command = { cmd, data };

	commandQueue.put(command);
	this->notify();
}

bool WiFi::processCommand(Command cmd, void *data)
{       // This function should be running only from one thread
	assert_param(cmd < COMMAND_NUM);
	processInProgress.lock();
	bool result = (this->*(handler[cmd]))(data);
	processInProgress.unlock();
	return result;
}
