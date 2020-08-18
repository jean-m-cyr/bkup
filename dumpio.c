#include "dumpio.h"

#include "globals.h"
#include "print.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <zlib.h>

static int fh = -1;
static gzFile gz_fh = NULL;

void dump_open(void)
{
    if (flags.compress)
    {
        gz_fh = gzdopen(fileno(flags.backup ? stdout : stdin), flags.backup ? "wb" : "rb");
        if (gz_fh != NULL)
            return;
    }
    else
    {
        fh = fileno(stdout);
        if (fh >= 0)
            return;
    }
    error("Can't open dump file\n%s\n", strerror(errno));
}

void dump_read(void* buffer, uint32_t bytes)
{
    if (flags.compress)
    {
        if (gzread(gz_fh, buffer, (uint64_t)bytes) == bytes)
            return;
    }
    else
    {
        if (read(fh, buffer, (uint64_t)bytes) == bytes)
            return;
    }
    error("Dump read failed\n%s\n", strerror(errno));
}

void dump_write(void* buffer, uint32_t bytes)
{
    if (flags.compress)
    {
        if (gzwrite(gz_fh, buffer, (uint64_t)bytes) == bytes)
            return;
    }
    else
    {
        if (write(fh, buffer, (uint64_t)bytes) == bytes)
            return;
    }
    error("Dump write failed\n%s\n", strerror(errno));
}

void dump_close(void)
{
    if (gz_fh)
        gzclose(gz_fh);
    if (fh >= 0)
        close(fh);
}
