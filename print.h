#pragma once

#include <stdarg.h>
#include <stddef.h>

void print(char* fmt, ...);
void error(char* fmt, ...);
void* alloc(size_t s);
void release(void* m);
