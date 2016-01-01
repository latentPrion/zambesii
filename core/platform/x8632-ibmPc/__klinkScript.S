
#include <in_asm.h>
/* Need to include the platform's virtual and physical base address information.
 */
#include <arch/x8632/paging.h>
#include <platform/x8632-ibmPc/memory.h>
#include <platform/x8632-ibmPc/cpu.h>

#define IBMPC_LOWMEM_TRAMPOLINE_NEXT_PADDR(_section)		\
	(CHIPSET_MEMORY___KLOAD_PADDR_BASE \
		+ (ADDR(_section) - x8632_IBMPC_POWERON_PADDR_BASE))

#define IBMPC___KLOAD_PADDR_NEXT_PADDR(_section)			\
	(ADDR(_section))

#define IBMPC___KLOAD_VADDR_NEXT_PADDR(_section)			\
	(CHIPSET_MEMORY___KLOAD_PADDR_BASE \
		+ (ADDR(_section) - ARCH_MEMORY___KLOAD_VADDR_BASE))

OUTPUT_FORMAT("elf32-i386", "elf32-i386",
              "elf32-i386")
ENTRY(_start)

/*INCLUDE ldSearchDirs.ld*/

MEMORY
{
	ibmPc_lowmem_trampoline (rx)
		: ORIGIN = x8632_IBMPC_POWERON_PADDR_BASE, LENGTH = 16K
	ibmPc___kload_paddr (rwx)
		: ORIGIN = CHIPSET_MEMORY___KLOAD_PADDR_BASE, LENGTH = 4M
	ibmPc___kload_vaddr (rwx)
		: ORIGIN = ARCH_MEMORY___KLOAD_VADDR_BASE, LENGTH = 1020M
}

SECTIONS
{
	/*	EXPLANATION:
	 * These sections are virtually linked to the IBM-PC lowmem
	 * region. Specifically the area within lowmem reserved for
	 * the AP wakeup trampoline code and data.
	 **********************************************************************/

	. = x8632_IBMPC_POWERON_PADDR_BASE;

	.__kheaders : AT(CHIPSET_MEMORY___KLOAD_PADDR_BASE) {
		*(.__kheaders*)
	} >ibmPc_lowmem_trampoline

	.__kcpuPowerOn.data BLOCK(0x4)
	: AT(IBMPC_LOWMEM_TRAMPOLINE_NEXT_PADDR(.__kcpuPowerOn.data)) {
		__kcpuPowerOnDataStart = .;
		*(.__kcpuPowerOnData*)
		__kcpuPowerOnDataEnd = .;
	} >ibmPc_lowmem_trampoline

	.__kcpuPowerOn.text BLOCK(PAGING_BASE_SIZE)
	: AT(IBMPC_LOWMEM_TRAMPOLINE_NEXT_PADDR(.__kcpuPowerOn.text)) {
		__kcpuPowerOnTextStart = .;
		*(.__kcpuPowerOnText*)
		__kcpuPowerOnTextEnd = .;
	} >ibmPc_lowmem_trampoline


	/*	EXPLANATION:
	 * These sections are linked to be identity-mapped to the
	 * physical location that they are loaded to in actual
	 * physical RAM.
	 *
	 * They must be identity mapped because they contain code
	 * and data which needs to yield their actual paddr when referenced.
	 * Examples of such code/data include:
	 *	* Page tables.
	 *	* Code that is executed immediately after enabling paging.
	 **********************************************************************/

	. = CHIPSET_MEMORY___KLOAD_PADDR_BASE
		+ (. - x8632_IBMPC_POWERON_PADDR_BASE);

	.__ksetup.data BLOCK(PAGING_BASE_SIZE)
	: AT(IBMPC___KLOAD_PADDR_NEXT_PADDR(.__ksetup.data)) {
		__ksetupDataStart = .;
		*(.__ksetupData*)
		__ksetupDataEnd = .;
	} >ibmPc___kload_paddr

	.__ksetup.text BLOCK(0x4)
	: AT(IBMPC___KLOAD_PADDR_NEXT_PADDR(.__ksetup.text)) {
		*(.__ksetupText*)
	} >ibmPc___kload_paddr


	/*	EXPLANATION:
	 * These sections are linked to the kernel's high load
	 * vaddr.
	 *
	 * I.e, these sections are linked normally. There is no
	 * peculiarity about them.
	 **********************************************************************/

	. = ARCH_MEMORY___KLOAD_VADDR_BASE + (. - CHIPSET_MEMORY___KLOAD_PADDR_BASE);

	__kvirtStart = .;
	__kloadVaddrCodeStart = .;

	.text BLOCK(PAGING_BASE_SIZE)
	: AT(IBMPC___KLOAD_VADDR_NEXT_PADDR(.text)) {
		*(.text*)
	} >ibmPc___kload_vaddr

	__kloadVaddrCodeEnd = .;
	__kloadVaddrRodataStart = .;

	.init BLOCK(4)
	: AT(IBMPC___KLOAD_VADDR_NEXT_PADDR(.init)) {
		*(.init.prologue)
		*(.init)
		*(.init.epilogue)
	} >ibmPc___kload_vaddr

	.fini BLOCK(4)
	: AT(IBMPC___KLOAD_VADDR_NEXT_PADDR(.fini)) {
		*(.fini.prologue)
		*(.fini)
		*(.fini.epilogue)
	} >ibmPc___kload_vaddr

	.ctor BLOCK(PAGING_BASE_SIZE)
	: AT(IBMPC___KLOAD_VADDR_NEXT_PADDR(.ctor)) {
		__kctorStart = .;
		*(.ctor*)
		__kctorEnd = .;
	} >ibmPc___kload_vaddr

	.dtor BLOCK(0x4)
	: AT(IBMPC___KLOAD_VADDR_NEXT_PADDR(.dtor)) {
		__kdtorStart = .;
		*(.dtor*)
		__kdtorEnd = .;
	} >ibmPc___kload_vaddr

	.gnu.linkonce.t BLOCK(0x4)
	: AT(IBMPC___KLOAD_VADDR_NEXT_PADDR(.gnu.linkonce.t)) {
		*(.gnu.linkonce.t*)
	} >ibmPc___kload_vaddr

	.rodata BLOCK(0x4)
	: AT(IBMPC___KLOAD_VADDR_NEXT_PADDR(.rodata)) {
		*(.rodata*)
	} >ibmPc___kload_vaddr

	__kloadVaddrRodataEnd = .;

	/** NOTE:
	 * The following sections are read-write.
	 **/

	__kloadVaddrDataStart = .;

	.data BLOCK(PAGING_BASE_SIZE)
	: AT(IBMPC___KLOAD_VADDR_NEXT_PADDR(.data)) {
		*(.data*)

		. = ALIGN(0x4);
		*(COMMON*)
	} >ibmPc___kload_vaddr

	.bss BLOCK(0x4)
	: AT(IBMPC___KLOAD_VADDR_NEXT_PADDR(.bss)) {
		__kbssStart = .;
		*(.bss*)
		__kbssEnd = .;
	} >ibmPc___kload_vaddr

	__kloadVaddrDataEnd = .;

	__kvirtEnd = .;
	__kphysStart = CHIPSET_MEMORY___KLOAD_PADDR_BASE;
	/*__kphysEnd = CHIPSET_MEMORY___KLOAD_PADDR_BASE + (ARCH_MEMORY___KLOAD_VADDR_BASE - .);*/
	__kphysEnd = . - ARCH_MEMORY___KLOAD_VADDR_BASE;
	__kend = .;

	/DISCARD/ :
	{
		*(.comment)
	}
}
