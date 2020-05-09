/* (c) 2020 ukrkyi */
#ifndef SYSTEM_H
#define SYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif
	void SystemClock_Config(void);
	void SystemConfig(void);
#ifdef __cplusplus
}
#endif

#define UART_RX_IT_PRIORITY	5
#define UART_TX_IT_PRIORITY	10
#define ULTRASONIC_IT_PRIORITY	6

#endif // SYSTEM_H
