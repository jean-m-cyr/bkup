#include "mbr.h"

#include "devio.h"
#include "dumpio.h"
#include "globals.h"
#include "print.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MSDOS_LABEL_MAGIC 0xAA55

typedef struct __attribute__((__packed__)) _gpt_mbr_record
{
    uint8_t boot_indicator; /* unused by EFI, set to 0x80 for bootable */
    uint8_t start_head;     /* unused by EFI, pt start in CHS */
    uint8_t start_sector;   /* unused by EFI, pt start in CHS */
    uint8_t start_track;
    uint8_t os_type;       /* EFI and legacy non-EFI OS types */
    uint8_t end_head;      /* unused by EFI, pt end in CHS */
    uint8_t end_sector;    /* unused by EFI, pt end in CHS */
    uint8_t end_track;     /* unused by EFI, pt end in CHS */
    uint32_t starting_lba; /* used by EFI - start addr of the on disk pt */
    uint32_t size_in_lba;  /* used by EFI - size of pt in LBA */
} gpt_mbr_record_t;

typedef struct __attribute__((__packed__)) _legacy_mbr
{
    uint8_t boot_code[440];
    uint32_t unique_mbr_signature;
    uint16_t unknown;
    gpt_mbr_record_t partition_record[4];
    uint16_t signature;
} legacy_mbr_t;

static dump_header_t hdr;

static char* part_type(uint8_t os_type)
{
    switch (os_type)
    {
    case FAT32_WITH_LBA_FS:
        return "FAT32";
    case NATIVE_LINUX_FS:
        return "Linux";
    }
    return "Unknown";
}


void mbr_load(void)
{
    void* sec0 = alloc(sector_size);
    dev_seek(0);
    dev_read(sec0, 1);
    legacy_mbr_t* mbr = sec0;
    if (mbr->signature != le16(MSDOS_LABEL_MAGIC))
        error("Unsupported master boot record\n");
    if (flags.debug)
    {
        print(
            "\nMBR signature: %04x\n"
            "unique MBR signature: %08x\n",
            le16(mbr->signature), le32(mbr->unique_mbr_signature));
        for (int p = 0; p < 4; p++)
        {
            gpt_mbr_record_t* part = mbr->partition_record + p;
            if (part->os_type == 0)
                continue;
            print("\nPartition %d\n", p);
            print(
                "boot inicator: %02x\n"
                "start head:    %02x\n"
                "start sector:  %02x\n"
                "start track:   %02x\n"
                "OS type:       %s\n"
                "end head:      %02x\n"
                "end sector:    %02x\n"
                "end track:     %02x\n"
                "starting LBA:  %08x\n"
                "size in lba:   %08x\n",
                part->boot_indicator, part->start_head, part->start_sector,
                part->start_track, part_type(part->os_type), part->end_head,
                part->end_sector, part->end_track, le32(part->starting_lba),
                le32(part->size_in_lba));
        }
    }

    memset(&hdr, 0, sizeof(hdr));
    hdr.sector_size = le16(sector_size);
    hdr.device_size = le32(device_size);

    for (int p = 0; p < 4; p++)
    {
        gpt_mbr_record_t* part = mbr->partition_record + p;
        if (part->os_type == 0)
            continue;
        hdr.short_part[hdr.partitions].os_type = part->os_type;
        hdr.short_part[hdr.partitions].start_lba = le32(part->starting_lba);
        hdr.short_part[hdr.partitions].size_lba = le32(part->size_in_lba);
        hdr.short_part[hdr.partitions].part_no = p + 1;
        hdr.partitions++;
    }

    if (flags.debug)
    {
        for (int p = 0; p < mbr_part_count(); p++)
        {
            print("\nshort partition %d\n", p);
            print(
                "OS type:       %s\n"
                "partition #:   %d\n"
                "starting LBA:  %08x\n"
                "size in lba:   %08x\n",
                mbr_part_type_name(p), mbr_part_no(p), mbr_part_start_lba(p),
                mbr_part_lba_size(p));
        }
    }
    dump_write(&hdr, sizeof(hdr));
    dump_write(sec0, sector_size);
    release(sec0);
}

uint32_t mbr_part_count(void)
{
    return hdr.partitions;
}

uint8_t mbr_part_type(int part)
{
    return hdr.short_part[part].os_type;
}

char* mbr_part_type_name(int part)
{
    return part_type(hdr.short_part[part].os_type);
}

uint32_t mbr_part_start_lba(int part)
{
    return hdr.short_part[part].start_lba;
}

uint32_t mbr_part_lba_size(int part)
{
    return hdr.short_part[part].size_lba;
}

uint8_t mbr_part_no(int part)
{
    return hdr.short_part[part].part_no;
}
