#pragma once

#include <stdint.h>

void dev_open(char* fn);
void dev_seek(uint64_t sector);
void dev_read(void* buffer, uint32_t sectors);
void dev_write(void* buffer, uint32_t sectors);
void dev_close(void);
