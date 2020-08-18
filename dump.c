#include "dump.h"

#include "dump_ext.h"
#include "dump_fat32.h"
#include "dump_unknown.h"
#include "mbr.h"
#include "print.h"

void dump(void)
{
    mbr_load();
    for (int p = 0; p < mbr_part_count(); p++)
    {
        print("\nDumping %s (partition %d)\n", mbr_part_type_name(p), mbr_part_no(p));
        switch (mbr_part_type(p))
        {
        case FAT32_WITH_LBA_FS:
            dump_fat32(mbr_part_start_lba(p), mbr_part_lba_size(p));
            break;
        case NATIVE_LINUX_FS:
            dump_ext(mbr_part_start_lba(p), mbr_part_lba_size(p));
            break;
        default:
            dump_unknown(mbr_part_start_lba(p), mbr_part_lba_size(p));
            break;
        }
    }
}
