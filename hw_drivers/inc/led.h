/* (c) 2020 ukrkyi */
#ifndef LED_H
#define LED_H


class LED
{
	LED();
	~LED();
public:
	LED(const LED&) = delete;
	void blink(unsigned n, unsigned wait_till = 0);
	void on();
	void off();
	void toggle();
	bool is_on();
	static LED& getInstance();
};

#endif // LED_H
