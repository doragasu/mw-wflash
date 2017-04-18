#include <stdint.h>

#ifndef NULL
#define NULL (void*)0;
#endif

// main function
extern int main(uint16_t hard);

typedef void(*_voidCallback)(void);

void _start_entry()
{
    // Call main program
    main(1);
}

void _reset_entry()
{
    main(0);
}

