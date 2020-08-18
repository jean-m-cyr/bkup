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
    if (flags.debug > 1)
        print("dev_open(\"%s\")\n", fn);
    fh = open(fn, flags.backup ? O_RDONLY : O_WRONLY);
    if (fh < 0)
        error("Can't open %s\n%s\n", fn, strerror(errno));
}

void dev_seek(uint64_t sector)
{
    if (flags.debug > 1)
        print("dev_seek(%llu)\n", sector);
    uint64_t offset = sector * sector_size;
    if (lseek(fh, offset, SEEK_SET) != offset)
        error("Device seek failed at offset %'llu\n%s\n", offset, strerror(errno));
}

void dev_read(void* buffer, uint32_t sectors)
{
    if (flags.debug > 1)
        print("dev_read(%u)\n", sectors);
    uint64_t bytes = (uint64_t)sectors * sector_size;
    if (read(fh, buffer, bytes) != bytes)
        error("Device read failed\n%s\n", strerror(errno));
}

void dev_write(void* buffer, uint32_t sectors)
{
    if (flags.debug > 1)
        print("dev_write(%u)\n", sectors);
    uint64_t bytes = (uint64_t)sectors * sector_size;
    if (write(fh, buffer, bytes) != bytes)
        error("Device read failed\n%s\n", strerror(errno));
}

void dev_close(void)
{
    if (flags.debug > 1)
        print("dev_close()\n");
    close(fh);
}
