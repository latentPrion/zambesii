#include <in_asm.h>
//Include the memory information for this build of the kernel. We need the Vaddr and the Paddr for this linker script.
#include <platform/x8632-ibmPc/memory.h>
#include <arch/x8632/paging.h>

/* The C preprocessor in GCC seems to define a symbol 'i386' and give it a value of 1.
 * This causes the 'i386' in OUTPUT_ARCH() to get macro expanded and give syntax errors.
 **/
//#define i386	i386
ENTRY(__korientationAsmEntry)
/*
OUTPUT_ARCH(i386)
OUTPUT_FORMAT(elf)
TARGET(i386-elf)
*/


SECTIONS
{
	/* Start off with the physical sections */
	. = CHIPSET_MEMORY___KLOAD_PADDR_BASE;
	__kstart = .;

	__kphysStart = .;
	.__kheaders ALIGN(PAGING_BASE_SIZE) : AT(CHIPSET_MEMORY___KLOAD_PADDR_BASE)
	{
		__kheadersStart = .;
		*(.__kheaders*)
		__kheadersEnd = .;
	}

	.__ksetup ALIGN(PAGING_BASE_SIZE) : AT(ADDR(.__ksetup))
	{
		__ksetupDataStart = .;
		*(.__ksetupData*)
		__ksetupDataEnd = .;
		__ksetupTextStart = .;
		*(.__ksetupText*)
		*(.__ksetup*)
		__ksetupTextEnd = .;
	}
	__kphysEnd = .;

	. = (__kphysEnd - CHIPSET_MEMORY___KLOAD_PADDR_BASE) + ARCH_MEMORY___KLOAD_VADDR_BASE;

	__kvirtStart = .;
	.__korientation ALIGN(PAGING_BASE_SIZE) : AT( ADDR(.__korientation) - ARCH_MEMORY___KLOAD_VADDR_BASE + CHIPSET_MEMORY___KLOAD_PADDR_BASE)
	{
		/* Include the .__korientationText and .__korientationData sections for all files here. */
		__korientationStart = .;
		*(.__korientationText*)
		*(.__korientationData*)
		*(.__korientation*)
		__korientationEnd = .;
	}

	.init ALIGN(PAGING_BASE_SIZE) : AT( ADDR(.init) - ARCH_MEMORY___KLOAD_VADDR_BASE + CHIPSET_MEMORY___KLOAD_PADDR_BASE)
	{
		__kinitStart = .;
		*(.init*)
		__kinitEnd = .;
	}

	.ctor ALIGN(PAGING_BASE_SIZE) : AT( ADDR(.ctor) - ARCH_MEMORY___KLOAD_VADDR_BASE + CHIPSET_MEMORY___KLOAD_PADDR_BASE)
	{
		__kctorStart = .;
		*(.ctor*)
		__kctorEnd = .;
	}

	.text ALIGN(PAGING_BASE_SIZE) : AT( ADDR(.text) - ARCH_MEMORY___KLOAD_VADDR_BASE + CHIPSET_MEMORY___KLOAD_PADDR_BASE)
	{
		__ktextStart = .;
		*(.text*)
		__ktextEnd = .;
	}

	.data ALIGN(PAGING_BASE_SIZE) : AT( ADDR(.data) - ARCH_MEMORY___KLOAD_VADDR_BASE + CHIPSET_MEMORY___KLOAD_PADDR_BASE)
	{
		__kdataStart = .;
		*(.data*)
		. = ALIGN(0x4);
		*(.rodata*)
		__kdataEnd = .;
	}

	.fini ALIGN(PAGING_BASE_SIZE) : AT( ADDR(.fini) - ARCH_MEMORY___KLOAD_VADDR_BASE + CHIPSET_MEMORY___KLOAD_PADDR_BASE)
	{
		__kfiniStart = .;
		*(.fini*)
		__kdtorStart = .;
		*(.dtor*)
		__kdtorEnd = .;
		__kfiniEnd = .;
	}

	.bss ALIGN(PAGING_BASE_SIZE) : AT( ADDR(.bss) - ARCH_MEMORY___KLOAD_VADDR_BASE + CHIPSET_MEMORY___KLOAD_PADDR_BASE)
	{
		__kbssStart = .;
		*(.bss*)
		__kbssEnd = .;
		. = ALIGN(0x4);
		*(COMMON)
	}

	/DISCARD/ :
	{
		*(.comment)
		*(.eh_frame)
	}
	__kvirtEnd = .;

	__kend = .;
}

