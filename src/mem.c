#include <stdio.h>
#include <stdint.h>

////////////////////////////////////////////////////////////////
//
// void * memcpy(void *dst, const void *src, size_t len)
// copia un bloque de memoria src en dst de tamaÃ±o len
//
////////////////////////////////////////////////////////////////

void * memcpy(void *dst, const void *src, size_t len){

    size_t i;

    if ((uintptr_t)dst % sizeof(long) == 0 &&
    (uintptr_t)src % sizeof(long) == 0 &&
    len % sizeof(long) == 0) {

        long *d = dst;
        const long *s = src;

        for (i=0; i<len/sizeof(long); i++)
            d[i] = s[i];
    }
    else{
        char *d = dst;
        const char *s = src;

        for (i=0; i<len; i++)
            d[i] = s[i];
    }

    return dst;
}

//-----------------------------------------------------------------------------
// STRING COPY
//-----------------------------------------------------------------------------
char* strcpy(char *to, const char *from)
{
    const char *src;
    char *dst;

    src = from;
    dst = to;

    while ((*dst++ = *src++));

    return to;
}


//-----------------------------------------------------------------------------
// STRING LENGHT
//-----------------------------------------------------------------------------
unsigned long strlen(const char *str)
{
    const char *src;

    src = str;

    while (*src++);

    return (src - str) - 1;
}

