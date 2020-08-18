#pragma once

#include "globals.h"
#include "print.h"

#include <errno.h>

void dump_open(void);
void dump_read(void* buffer, uint32_t bytes);
void dump_write(void* buffer, uint32_t bytes);
void dump_close(void);
