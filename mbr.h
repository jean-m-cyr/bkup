#pragma once

#include <stdint.h>

#define FAT32_WITH_LBA_FS 0x0c
#define NATIVE_LINUX_FS 0x83
#define GPT_FS 0xee

typedef struct short_mbr_record
{
    uint8_t os_type;
    uint8_t part_no;
    uint8_t major_version;
    uint8_t minor_version;
    uint32_t spare;
    uint64_t start_lba;
    uint64_t size_lba;
} hdr_mbr_record_t;

typedef struct dump_header
{
    uint32_t sector_size;
    uint16_t last_boot_lba;
    uint8_t partitions;
    uint8_t spare;
    uint64_t device_size;
    hdr_mbr_record_t short_part[1];
} dump_header_t;

void mbr_load(void);
uint32_t mbr_part_count(void);
uint8_t mbr_part_type(int part);
char* mbr_part_type_name(int part);
uint32_t mbr_part_start_lba(int part);
uint32_t mbr_part_lba_size(int part);
uint8_t mbr_part_no(int part);
