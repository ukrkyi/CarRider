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

#define I2C_IT_PRIORITY 	5
#define UART_RX_IT_PRIORITY	6
#define ACC_DATA_IT_PRIORITY	7
#define MAG_DATA_IT_PRIORITY	7
#define ULTRASONIC_IT_PRIORITY	8
#define I2C_CPLT_IT_PRIORITY	9
#define UART_TX_IT_PRIORITY	10

#endif // SYSTEM_H
