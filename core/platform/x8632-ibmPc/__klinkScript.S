
#include <in_asm.h>

/* Include the memory information for this build of the kernel. We need the
 * Vaddr and the Paddr info from the kernel for this linker script.
 **/
#include <platform/x8632-ibmPc/memory.h>
#include <platform/x8632-ibmPc/cpu.h>
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
	. = PLATFORM_x86_32_POWERON_DATA_PADDR;

	__kphysStart = CHIPSET_MEMORY___KLOAD_PADDR_BASE;

	// __kheaders contains things like the zBoot and multiboot headers.
	.__kheaders ALIGN(PAGING_BASE_SIZE)
	:
	AT(CHIPSET_MEMORY___KLOAD_PADDR_BASE)
	{
		__kheadersStart = .;
		*(.__kheaders*)
		__kheadersEnd = .;
	}

	.__kcpuPowerOnData ALIGN(PAGING_BASE_SIZE)
	:
	AT(CHIPSET_MEMORY___KLOAD_PADDR_BASE)
	{
		__kcpuPowerOnDataStart = .;
		*(.__kcpuPowerOnData*)
		__kcpuPowerOnDataEnd = .;
	}

	.__kcpuPowerOnText ALIGN(PAGING_BASE_SIZE)
	:
	AT(CHIPSET_MEMORY___KLOAD_PADDR_BASE + (. - ADDR(.__kcpuPowerData))
	{
		__kcpuPowerOnTextStart = .;
		*(.__kcpuPowerOnText*)
		__kcpuPowerOnTextEnd = .;
	}

	/* The next few sections must be identical in physical and virtual
	 * link address.
	 **/
	. += (CHIPSET_MEMORY___KLOAD_PADDR_BASE - .)
		+ (. - PLATFORM_x86_32_POWERON_DATA_PADDR);

	__kstart = .;

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

	.ctor ALIGN(0x4)
	:
	AT( ADDR(.ctor) - ARCH_MEMORY___KLOAD_VADDR_BASE
		+ CHIPSET_MEMORY___KLOAD_PADDR_BASE)
	{
		__kctorStart = .;
		*(.ctor*)
		__kctorEnd = .;
	}

	.dtor ALIGN(0x4)
	:
	AT(ADDR(.dtor) - ARCH_MEMORY___KLOAD_VADDR_BASE
		+ CHIPSET_MEMORY___KLOAD_PADDR_BASE)
	{
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

	.gnu.linkonce.t ALIGN(0x4)
	:
	AT( ADDR(.gnu.linkonce.t) - ARCH_MEMORY___KLOAD_VADDR_BASE
		+ CHIPSET_MEMORY___KLOAD_PADDR_BASE)
	{
		*(.gnu.linkonce.t*)
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

	.gnu.linkonce.r ALIGN(0x4)
	:
	AT( ADDR(.gnu.linkonce.r) - ARCH_MEMORY___KLOAD_VADDR_BASE
		+ CHIPSET_MEMORY___KLOAD_PADDR_BASE)
	{
		*(.gnu.linkonce.r*)
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

