
#include <in_asm.h>

/* Include the memory information for this build of the kernel. We need the
 * Vaddr and the Paddr info from the kernel for this linker script.
 **/
#include <platform/x8632-ibmPc/memory.h>
#include <arch/x8632/paging.h>

/* The C preprocessor in GCC seems to define a symbol 'i386' and give it a 
 * value of 1. This causes the 'i386' in OUTPUT_ARCH() to get macro expanded 
 * and give syntax errors.
 **/
// #define i386	i386
// OUTPUT_ARCH(i386)
// OUTPUT_FORMAT(elf)
// TARGET(i386-elf)

// Absolute kernel entry for any platform. Kernel orientation thread.
ENTRY(__korientationEntry)

SECTIONS
{
	/* Start off with the physical sections */
	. = CHIPSET_MEMORY___KLOAD_PADDR_BASE;

	__kstart = .;

	__kphysStart = .;

	// __kheaders contains things like the zBoot and multiboot headers.
	.__kheaders ALIGN(PAGING_BASE_SIZE) 
	:
	AT(CHIPSET_MEMORY___KLOAD_PADDR_BASE)
	{
		__kheadersStart = .;
		*(.__kheaders*)
		__kheadersEnd = .;
	}

	.__ksetupData ALIGN(PAGING_BASE_SIZE) : AT(ADDR(.__ksetupData))
	{
		__ksetupDataStart = .;
		*(.__ksetupData*)
		__ksetupDataEnd = .;
	}

	.__ksetupText ALIGN(0x4) : AT(ADDR(.__ksetupText))
	{
		__ksetupTextStart = .;
		*(.__ksetupText*)
		__ksetupTextEnd = .;
	}

	__kphysEnd = .;

	. = (__kphysEnd - CHIPSET_MEMORY___KLOAD_PADDR_BASE)
		+ ARCH_MEMORY___KLOAD_VADDR_BASE;
	. = ALIGN(PAGING_BASE_SIZE);

	__kvirtStart = .;

	__kreadOnlyStart = .;

	.__korientationText ALIGN(0x4)
	:
	AT( ADDR(.__korientationText) - ARCH_MEMORY___KLOAD_VADDR_BASE
		+ CHIPSET_MEMORY___KLOAD_PADDR_BASE)
	{
		__korientationTextStart = .;
		*(__korientationText*)
		__korientationTextEnd = .;
	}

	.__korientationData ALIGN(0x4)
	:
	AT( ADDR(.__korientationData) - ARCH_MEMORY___KLOAD_VADDR_BASE
		+ CHIPSET_MEMORY___KLOAD_PADDR_BASE)
	{
		__korientationDataStart = .;
		*(__korientationData*)
		__korientationDataEnd = .;
	}

	.cdtortl ALIGN(0x4)
	:
	AT( ADDR(.cdtortl) - ARCH_MEMORY___KLOAD_VADDR_BASE
		+ CHIPSET_MEMORY___KLOAD_PADDR_BASE)
	{
		__kctorStart = .;
		*(.ctor*)
		__kctorEnd = .;
		__kdtorStart = .;
		*(.dtor*)
		__kdtorEnd = .;
	}

	.text ALIGN(0x4)
	:
	AT( ADDR(.text) - ARCH_MEMORY___KLOAD_VADDR_BASE
		+ CHIPSET_MEMORY___KLOAD_PADDR_BASE)
	{
		__ktextStart = .;
		*(.text*)
		__ktextEnd = .;
	}

	.rodata ALIGN(0x4)
	:
	AT( ADDR(.rodata) - ARCH_MEMORY___KLOAD_VADDR_BASE
		+ CHIPSET_MEMORY___KLOAD_PADDR_BASE)
	{
		__kroDataStart = .;
		*(.rodata*)
		__krodataEnd = .;
	}

	__kreadOnlyEnd = .;

	. = ALIGN(PAGING_BASE_SIZE);
	__kreadWriteStart = .;

	.data
	:
	AT( ADDR(.data) - ARCH_MEMORY___KLOAD_VADDR_BASE
		+ CHIPSET_MEMORY___KLOAD_PADDR_BASE)
	{
		__kdataStart = .;
		*(.data*)
		__kdataEnd = .;

		. = ALIGN(0x4);
		__kbssStart = .;
		*(.bss*)
		__kbssEnd = .;
		. = ALIGN(0x4);
		*(COMMON)
	}

	__kreadWriteEnd = .;

	/DISCARD/ :
	{
		*(.comment)
		*(.eh_frame)
	}

	__kvirtEnd = .;

	__kend = .;
}

