/* (c) 2020 ukrkyi */
#ifndef LED_H
#define LED_H


class LED
{
public:
	LED();
	~LED();
	void on();
	void off();
	void toggle();
	bool is_on();
};

#endif // LED_H
