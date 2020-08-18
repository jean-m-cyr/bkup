#include "dump.h"

#include "mbr.h"
#include "print.h"

void dump(void)
{
    mbr_load();
    for (int p = 0; p < mbr_part_count(); p++)
    {
        print("\nDumping %s partition %d\n", mbr_part_type_name(p),
            mbr_part_no(p));
        switch (mbr_part_type(p))
        {
        case FAT32_WITH_LBA_FS:
            break;
        case NATIVE_LINUX_FS:
            break;
        default:
            break;
        }
    }
}
