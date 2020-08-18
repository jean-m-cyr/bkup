#include "devio.h"

#include "globals.h"
#include "print.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static int fh;

void dev_open(char* fn)
{
    fh = open(fn, flags.backup ? O_RDONLY : O_WRONLY);
    if (fh < 0)
        error("Can't open %s\n%s\n", fn, strerror(errno));
}

void dev_seek(uint32_t sector)
{
    uint64_t offset = (uint64_t)sector * sector_size;
    if (lseek(fh, offset, SEEK_SET) != offset)
        error("Device seek failed at offset %'llu\n%s\n", offset,
            strerror(errno));
}

void dev_read(void* buffer, uint32_t sectors)
{
    if (read(fh, buffer, (uint64_t)sectors * sector_size) !=
        (uint64_t)sectors * sector_size)
        error("Device read failed\n%s\n", strerror(errno));
}

void dev_write(void* buffer, uint32_t sectors)
{
    if (write(fh, buffer, (uint64_t)sectors * sector_size) !=
        (uint64_t)sectors * sector_size)
        error("Device read failed\n%s\n", strerror(errno));
}

void dev_close(void)
{
    close(fh);
}
