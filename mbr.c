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

typedef struct __attribute__((__packed__)) gpt_mbr_record
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

typedef struct __attribute__((__packed__)) legacy_mbr
{
    uint8_t boot_code[440];
    uint32_t unique_mbr_signature;
    uint16_t unknown;
    gpt_mbr_record_t partition_record[4];
    uint16_t signature;
} legacy_mbr_t;

typedef struct __attribute__((__packed__)) gpt_header
{
    uint64_t signature;
    uint32_t revision;
    uint32_t header_size;
    uint32_t crc32;
    uint32_t reserved;
    uint64_t current_lba;
    uint64_t backup_lba;
    uint64_t first_usable_lba;
    uint64_t last_usable_lba;
    char guid[16];
    uint64_t partition_entries_lba;
    uint32_t partition_entries;
    uint32_t partition_entry_size;
    uint32_t partitions_crc32;
} gpt_header_t;

typedef struct __attribute__((__packed__)) GUID_partition_entry
{
    char guid[16];
    char unique_guid[16];
    uint64_t first_lba;
    uint64_t last_lba;
    uint64_t attributes;
    char name[72];
} GUID_partition_entry_t;

static dump_header_t* hdr = NULL;

static char* part_type(uint8_t os_type)
{
    switch (os_type)
    {
    case FAT32_WITH_LBA_FS:
        return "FAT32 with LBA";
    case NATIVE_LINUX_FS:
        return "Linux native";
    case GPT_FS:
        return "GPT protective";
    }
    return "Unknown";
}


void mbr_load(void)
{
    void* sec = alloc(sector_size);
    dev_seek(0);
    dev_read(sec, 1);
    legacy_mbr_t* mbr = sec;
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
                part->boot_indicator, part->start_head, part->start_sector, part->start_track,
                part_type(part->os_type), part->end_head, part->end_sector, part->end_track,
                le32(part->starting_lba), le32(part->size_in_lba));
        }
    }

    int count = 0;
    for (int p = 0; p < 4; p++)
    {
        gpt_mbr_record_t* part = mbr->partition_record + p;
        if (part->os_type == 0)
            continue;
        count++;
    }

    size_t hdr_size;

    if (mbr->partition_record[0].os_type != GPT_FS)
    {
        hdr_size = sizeof(hdr) + sizeof(hdr_mbr_record_t) * (count - 1);
        hdr = alloc(hdr_size);
        memset(hdr, 0, hdr_size);
        hdr->sector_size = le16(sector_size);
        hdr->device_size = le32(device_size);

        for (int p = 0; p < 4; p++)
        {
            gpt_mbr_record_t* part = mbr->partition_record + p;
            if (part->os_type == 0)
                continue;
            hdr->short_part[hdr->partitions].os_type = part->os_type;
            hdr->short_part[hdr->partitions].start_lba = le32(part->starting_lba);
            hdr->short_part[hdr->partitions].size_lba = le32(part->size_in_lba);
            hdr->short_part[hdr->partitions].part_no = p + 1;
            hdr->partitions++;
        }
    }
    else
    {
        dev_seek(1);
        dev_read(sec, 1);
        gpt_header_t* gpt = sec;

        if (gpt->signature != le64(0x5452415020494645ULL))
            error("Invalid GPT header");

        if (flags.debug)
        {
            print(
                "\nGPT header\n"
                "signature:             %016llx\n"
                "revision:              %08x\n"
                "header_size:           %08x\n"
                "crc32:                 %08x\n"
                "current_lba:           %016llx\n"
                "backup_lba:            %016llx\n"
                "first_usable_lba       %016llx\n"
                "last_usable_lba        %016llx\n"
                "partition_entries_lba: %016llx\n"
                "partition_entries:     %08x\n"
                "partition_entry_size:  %08x\n"
                "partitions_crc32:      %08x\n",
                gpt->signature, gpt->revision, gpt->header_size, gpt->crc32, gpt->current_lba,
                gpt->backup_lba, gpt->first_usable_lba, gpt->last_usable_lba,
                gpt->partition_entries_lba, gpt->partition_entries, gpt->partition_entry_size,
                gpt->partitions_crc32);
        }

        size_t gpt_parts_size = gpt->partition_entries * gpt->partition_entry_size;
        GUID_partition_entry_t* gpt_parts = alloc(gpt_parts_size);
        dev_seek(gpt->partition_entries_lba);
        dev_read(gpt_parts, gpt_parts_size / sector_size);
        for (count = 0; count < gpt->partition_entries; count++)
        {
            static char null_guid[16] = {0};
            GUID_partition_entry_t* part = gpt_parts + count;
            if (memcmp(part->guid, null_guid, sizeof(null_guid)) == 0)
                break;
        }
        hdr_size = sizeof(hdr) + sizeof(hdr_mbr_record_t) * (count - 1);
        hdr = alloc(hdr_size);
        memset(hdr, 0, hdr_size);
        hdr->sector_size = le32(sector_size);
        hdr->device_size = le64(device_size);
        for (uint16_t p = 0; p < count; p++)
        {
            GUID_partition_entry_t* part = gpt_parts + p;
            if (flags.debug)
            {
                print(
                    "\nGPT Partition %d\n"
                    "guid:        %016llx%016llx\n"
                    "unique_guid: %016llx%016llx\n"
                    "first LBA:   %016llx\n"
                    "last LBA:    %016llx\n"
                    "attributes:  %016llx\n"
                    "name:        ",
                    p, *((uint64_t*)part->guid), *((uint64_t*)&part->guid[8]),
                    *((uint64_t*)part->unique_guid), *((uint64_t*)&part->unique_guid[8]),
                    le64(part->first_lba), le64(part->last_lba), le64(part->attributes));
                for (int i = 0; i < 72; i += 2)
                {
                    if (part->name[i] == 0)
                        break;
                    print("%c", part->name[i]);
                }
                print("\n");
            }
            hdr->short_part[hdr->partitions].os_type = NATIVE_LINUX_FS;
            hdr->short_part[hdr->partitions].start_lba = le64(part->first_lba);
            hdr->short_part[hdr->partitions].size_lba = le64(part->last_lba - part->first_lba + 1);
            hdr->short_part[hdr->partitions].part_no = p + 1;
            hdr->partitions++;
        }
        hdr->last_boot_lba = gpt->first_usable_lba - 1;
        release(gpt_parts);
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
                mbr_part_type_name(p), mbr_part_no(p), mbr_part_start_lba(p), mbr_part_lba_size(p));
        }
    }
    dump_write(hdr, hdr_size);
    release(hdr);
    dev_seek(0);
    for (uint16_t s = 0; s <= hdr->last_boot_lba; s++)
    {
        dev_read(sec, 1);
        dump_write(sec, sector_size);
    }
    release(sec);
}

uint32_t mbr_part_count(void)
{
    return hdr->partitions;
}

uint8_t mbr_part_type(int part)
{
    return hdr->short_part[part].os_type;
}

char* mbr_part_type_name(int part)
{
    return part_type(hdr->short_part[part].os_type);
}

uint32_t mbr_part_start_lba(int part)
{
    return hdr->short_part[part].start_lba;
}

uint32_t mbr_part_lba_size(int part)
{
    return hdr->short_part[part].size_lba;
}

uint8_t mbr_part_no(int part)
{
    return hdr->short_part[part].part_no;
}
