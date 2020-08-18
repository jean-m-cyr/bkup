#include "devio.h"
#include "dump.h"
#include "dumpio.h"
#include "globals.h"
#include "print.h"
#include "restore.h"

#include <fcntl.h>
#include <getopt.h>
#include <locale.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

uint16_t sector_size;
uint32_t device_size;
flags_t flags = {0};

static char* dev_fn = NULL;
static char* full_fn = NULL;

static void help(char* prog)
{
    print(
        "\nUsage: %s [-c] [-h] [-d n] block_device_path\n   -c  enable "
        "compression\n   "
        "-h  display this help\n   -d n  enable debug info\n\n",
        prog);
    exit(-1);
}

static void parse_args(int ac, char* av[])
{
    char* who = strrchr(av[0], '/');
    if (who)
        who++;
    else
        who = av[0];
    if (strcmp(who, "bkup") == 0)
        flags.backup = 1;

    print("\n%s block device\n", flags.backup ? "Backup" : "Restore");

    if (ac < 2)
        help(av[0]);

    int c;
    opterr = 0;

    while ((c = getopt(ac, av, "chd:")) != -1)
        switch (c)
        {
        case 'c':
            flags.compress = 1;
            break;
        case 'd':
            flags.debug = atoi(optarg);
            break;
        default:
            print("Unknown argument %c\n", c);
        case 'h':
            help(av[0]);
        }

    if (optind >= ac)
    {
        print("Block device name missing\n");
        help(av[0]);
    }

    dev_fn = av[optind];

    full_fn = alloc(strlen(dev_fn) + 1 + 5);
    strcpy(full_fn, "/dev/");
    strcat(full_fn, dev_fn);

    if (access(full_fn, F_OK) < 0)
    {
        print("%s not found in /dev directory\n", full_fn);
        help(av[0]);
    }
}

static uint32_t getsysdata(char* fmt)
{
    char* fn = alloc((strlen(fmt) - 2) + strlen(dev_fn) + 1);
    snprintf(fn, (strlen(fmt) - 2) + strlen(dev_fn) + 1, fmt, dev_fn);
    int h = open(fn, O_RDONLY);
    if (h < 0)
    {
        release(fn);
        return 0;
    }
    char buf[16];
    ssize_t l = read(h, buf, sizeof(buf) - 1);
    buf[l] = 0;
    char* p;
    uint32_t r = strtoul(buf, &p, 10);
    close(h);
    release(fn);
    return r;
}

int main(int ac, char* av[])
{
    setlocale(LC_NUMERIC, "");
    setlocale(LC_TIME, "");

    parse_args(ac, av);

    time_t start_time = time(NULL);

    sector_size = getsysdata("/sys/block/%s/queue/hw_sector_size");
    if (sector_size == 0)
    {
        print("Unable to get sector size from /sys filesystem, assuming 512\n");
        sector_size = 512;
    }
    device_size = getsysdata("/sys/block/%s/size");
    if (device_size == 0)
    {
        print(
            "Unable to get device size from /sys filesystem, assuming "
            "infinity\n");
        device_size = -1;
    }

    print(
        "\n%sing block device /dev/%s, dev. sectors %'u, bytes per sector "
        "%'u\n",
        flags.backup ? "Dump" : "Restor", dev_fn, device_size, sector_size);

    dev_open(full_fn);
    dump_open();

    flags.backup ? dump() : restore();

    dump_close();
    dev_close();

    time_t elapsed = time(NULL) - start_time;
    uint8_t s = elapsed % 60;
    elapsed /= 60;
    uint8_t m = elapsed % 60;
    elapsed /= 60;
    print("\nElapsed time %u:%02u:%02u\n\n", elapsed, m, s);

    return 0;
}
