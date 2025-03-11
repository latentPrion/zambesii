#include <kernel/common/__koptimizationHacks.h>
#include <multiboot.h>
#include <multiboot2.h>

	/**	EXPLANATION:
	 * The kernel conforms to several specifications which may dictate a
	 * header, or other form of identification to be placed into the kernel
	 * file. Examples of such are the GNU Multiboot Specification's Multiboot
	 * Header, and the zBoot Kernel Header.
	 *
	 * This file defines these headers, and places them into the .__kheaders
	 * section of the kernel ELF.
	 **/

#define MULTIBOOT_HEADER_FLAGS \
	(MULTIBOOT_HEADER_NEED_MODULES_ALIGNED | MULTIBOOT_HEADER_NEED_MEMORY_MAP)

sMultibootHeader __attribute__(( section(".__kheaders"), aligned(4) ))
multibootHeader =
{
	MULTIBOOT_HEADER_MAGIC,
	/* We need nothing else from a multiboot bootloader. Technically, we don't
	 * even need these things, since we don't use module loading or init-rd,
	 * and we have our own abstraction for obtaining a memory map.
	 **/
	MULTIBOOT_HEADER_FLAGS,
	-(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS),
	0, 0, 0, 0, 0,
	0, 0, 0, 0
};

/* Multiboot 2 header */
__attribute__(( section(".__kheaders"), aligned(8) ))
struct sMultiboot2FilledOutHeader {
    sMultiboot2Header header;

    /* End tag */
    struct {
        sMultiboot2HeaderTag tag;
    } __attribute__((packed)) end_tag;
}
multiboot2Header =
{
    {
        /* Magic value */
        MULTIBOOT2_HEADER_MAGIC,
        /* Architecture - i386 */
        MULTIBOOT2_ARCHITECTURE_I386,
        /* Header length */
        sizeof(multiboot2Header),
        /* Checksum */
        -(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT2_ARCHITECTURE_I386 + sizeof(multiboot2Header))
    },

    /* End tag */
    {
        {
            MULTIBOOT2_HEADER_TAG_END,
            0, /* flags */
            8  /* size */
        }
    }
};

/* This function must be called in the __korientation, or else LD will optimize
 * the .__kheaders section out of the kernel file.
 **/
void __kheadersInit(void) {
	sMultibootHeader	*volatile mbheader = &multibootHeader;
	void                *volatile mb2header = &multiboot2Header;
	while (mbheader) {return;};
	while (mb2header) {return;};
	return;
}

