#ifndef _MULTIBOOT2_H
#define _MULTIBOOT2_H

#include <__kstdlib/__ktypes.h>

/* Multiboot2 header magic value */
#define MULTIBOOT2_HEADER_MAGIC                  0xE85250D6

/* Multiboot2 header architecture - i386 */
#define MULTIBOOT2_ARCHITECTURE_I386             0

/* Multiboot2 header length - must include entire header */
#define MULTIBOOT2_HEADER_TAG_ALIGN              8

/* Multiboot2 information structure magic value */
#define MULTIBOOT2_BOOTLOADER_MAGIC              0x36D76289

/* Multiboot2 header tag types */
#define MULTIBOOT2_HEADER_TAG_END                0

#ifndef __ASM__

/* Multiboot2 header structure */
struct sMultiboot2Header {
    ubit32 magic;
    ubit32 architecture;
    ubit32 header_length;
    ubit32 checksum;
};

/* Multiboot2 header tag structure */
struct sMultiboot2HeaderTag {
    ubit16 type;
    ubit16 flags;
    ubit32 size;
};

/* Multiboot2 information structure */
struct sMultiboot2Info {
    ubit32 total_size;
    ubit32 reserved;
};

#endif /* !defined( __ASM__ ) */

#endif /* _MULTIBOOT2_H */
