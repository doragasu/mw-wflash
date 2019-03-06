#include <stdint.h>

#ifndef NULL
#define NULL (void*)0;
#endif

// main function
extern int entry_point(uint16_t hard);

typedef void(*_voidCallback)(void);

void _start_entry()
{
	// Call main program
	entry_point(1);
}

void _reset_entry()
{
	entry_point(0);
}

