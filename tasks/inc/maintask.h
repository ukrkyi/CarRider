/* (c) 2020 ukrkyi */
#ifndef MAIN_H
#define MAIN_H

#include "task.hpp"

class Main : public Task
{
	Main(const char * name, UBaseType_t priority);
	void run();
public:

	static Main& getInstance();
};

#endif // MAIN_H
