#include "print.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print(char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fflush(stderr);
}

void error(char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "ERROR: ");
    vfprintf(stderr, fmt, args);
    va_end(args);
    exit(-1);
}

void* alloc(size_t s)
{
    void* r = malloc(s);
    if (r == NULL)
        error("Can't allocate memory\n%s\n", strerror(errno));
    return r;
}

void release(void* m)
{
    free(m);
}
