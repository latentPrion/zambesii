
#include <in_asm.h>
/* Need to include the platform's virtual and physical base address information.
 */
#include <arch/x8632/paging.h>
#include <platform/x8632-ibmPc/memory.h>
#include <platform/x8632-ibmPc/cpu.h>


ENTRY(__korientationEntry)

SECTIONS
{
	/** NOTE:
	 * These sections are linked in low physical memory as their virtual
	 * address. They hold the code that is used to wake up CPUs. This code
	 * must begin on a page boundary and be in low memory.
	 *
	 * We enlist the aid of the linker to link everything virtually to low
	 * memory in order to be able to just memcpy the already linked code
	 * and not have to do any extra work.
	 **/
	. = x8632_IBMPC_POWERON_PADDR_BASE;
	.__kheaders : AT(CHIPSET_MEMORY___KLOAD_PADDR_BASE) {
		*(.__kheaders*)
	}

	.__kcpuPowerOnData ALIGN(0x4) : AT(CHIPSET_MEMORY___KLOAD_PADDR_BASE + (ADDR(.__kcpuPowerOnData) - x8632_IBMPC_POWERON_PADDR_BASE))
	{
		__kcpuPowerOnDataStart = .;
		*(.__kcpuPowerOnData*)
		__kcpuPowerOnDataEnd = .;
	}

	.__kcpuPowerOnText ALIGN(PAGING_BASE_SIZE) : AT(CHIPSET_MEMORY___KLOAD_PADDR_BASE + (ADDR(.__kcpuPowerOnText) - x8632_IBMPC_POWERON_PADDR_BASE))
	{
		__kcpuPowerOnTextStart = .;
		*(.__kcpuPowerOnText*)
		__kcpuPowerOnTextEnd = .;
	}

	/** NOTE:
	 * These next sections must be virtually linked to the exact place
	 * they are in physical memory; In other words, their virtual addresses
	 * must correspond 1:1 with their actual physical locations.
	 *
	 * They are used for the kernel's Orientation thread, as well as any
	 * other code/data which must be identity mapped (such as certain boot
	 * page tables which have been hardcoded into the kernel image).
	 **/
	. = CHIPSET_MEMORY___KLOAD_PADDR_BASE + (. - x8632_IBMPC_POWERON_PADDR_BASE);

	.__ksetupData ALIGN(PAGING_BASE_SIZE): AT(ADDR(.__ksetupData))
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

	/** NOTE:
	 * The rest of the sections are all linked virtually to 0xc0000000
	 * (in other words, ARCH_MEMORY___KLOAD_VADDR_BASE).
	 *
	 * The read only sections are separated from the read/write sections.
	 * This is so that later on in the boot process, the kernel can mark
	 * all of its text/data sections as read-only/read-write/executable etc.
	 * as needed.
	 **/
	. = ARCH_MEMORY___KLOAD_VADDR_BASE + (. - CHIPSET_MEMORY___KLOAD_PADDR_BASE);
	__kvirtStart = ARCH_MEMORY___KLOAD_VADDR_BASE;
	__kreadOnlyStart = .;

	.__korientationText ALIGN(PAGING_BASE_SIZE) : AT(CHIPSET_MEMORY___KLOAD_PADDR_BASE + (ADDR(.__korientationText) - ARCH_MEMORY___KLOAD_VADDR_BASE))
	{
		__korientationTextStart = .;
		*(.__korientationText*)
		__korientationTextEnd = .;
	}

	.__korientationData ALIGN(0x4) : AT(CHIPSET_MEMORY___KLOAD_PADDR_BASE + (ADDR(.__korientationData) - ARCH_MEMORY___KLOAD_VADDR_BASE))
	{
		__korientationDataStart = .;
		*(.__korientationData*)
		__korientationDataEnd = .;
	}

	.ctor ALIGN(0x4) : AT(CHIPSET_MEMORY___KLOAD_PADDR_BASE + (ADDR(.ctor) - ARCH_MEMORY___KLOAD_VADDR_BASE))
	{
		__kctorStart = .;
		*(.ctor*)
		__kctorEnd = .;
	}

	.dtor ALIGN(0x4) : AT(CHIPSET_MEMORY___KLOAD_PADDR_BASE + (ADDR(.dtor) - ARCH_MEMORY___KLOAD_VADDR_BASE))
	{
		__kdtorStart = .;
		*(.dtor*)
		__kdtorEnd = .;
	}

	.text ALIGN(0x4) : AT(CHIPSET_MEMORY___KLOAD_PADDR_BASE + (ADDR(.text) - ARCH_MEMORY___KLOAD_VADDR_BASE))
	{
		__ktextStart = .;
		*(.text*)
		__ktextEnd = .;
	}

	.gnu.linkonce.t ALIGN(0x4) : AT(CHIPSET_MEMORY___KLOAD_PADDR_BASE + (ADDR(.gnu.linkonce.t) - ARCH_MEMORY___KLOAD_VADDR_BASE))
	{
		*(.gnu.linkonce.t*)
	}

	.rodata ALIGN(0x4): AT(CHIPSET_MEMORY___KLOAD_PADDR_BASE + (ADDR(.rodata) - ARCH_MEMORY___KLOAD_VADDR_BASE))
	{
		__kroDataStart = .;
		*(.rodata*)
		__kroDataEnd = .;
	}

	/** NOTE:
	 * The following sections are read-write.
	 **/
	.data ALIGN(PAGING_BASE_SIZE) : AT(CHIPSET_MEMORY___KLOAD_PADDR_BASE + (ADDR(.data) - ARCH_MEMORY___KLOAD_VADDR_BASE))
	{
		__kdataStart = .;
		*(.data*)
		__kdataEnd = .;

		. = ALIGN(0x4);
		__kbssStart = .;
		*(.bss*)
		__kbssEnd = .;

		. = ALIGN(0x4);
		*(COMMON*)
	}

	__kreadOnlyEnd = .;
	__kvirtEnd = .;
	__kphysStart = CHIPSET_MEMORY___KLOAD_PADDR_BASE;
	__kphysEnd = CHIPSET_MEMORY___KLOAD_PADDR_BASE + (ARCH_MEMORY___KLOAD_VADDR_BASE - .);
	__kend = .;

	/DISCARD/ :
	{
		*(.comment)
		*(.eh_frame)
	}
}

