#ifndef _PER_CHIPSET_TABLE_FINDING_CODE_H
	#define _PER_CHIPSET_TABLE_FINDING_CODE_H

	#include <scaling.h>
	#include <chipset/chipset.h>

/**	EXPLANATION:
 * This header declares a set of chipset specific functions that will search
 * memory on that chipset in a chipset specific manner for certain structures,
 * or other chipset specific entities that are a specific to that chipset, but
 * part of a larger, portable specification.
 *
 * Examples include:
 *	1. ACPI RSDP:
 * 	   This structure on the IBM-PC is found somewhere in low
 *	   memory. There is no guarantee that on any other chipset it will be
 *	   found in that same place. Therefore this location, low memory, is
 *	   specific to the IBM-PC's implementation of ACPI. However, ACPI is a
 *	   broad and portable enough specification that it would be stupid to
 *	   make a mess of it and do things like:
 *	   #ifdef THIS_CHIPSET
 *	   	do_this();
 *	   #else
 *	   	do_that();
 *	   #endif
 *
 *	   The obvious solution is to have each chipset find the structure in
 *	   its own chipset specific manner and parse present it to the ACPI
 *	   code.
 *
 *	2. x86 MP Floating Pointer Structure:
 *	   This structure is part of an overall x86 MP specification, but the
 *	   specification only details where it should be found on the IBM-PC.
 *
 *	   For the same reasons as those stated above for ACPI's RSDP therefore,
 *	   we require all chipsets that have x86, and require the kernel to do
 *	   MP to define their own chipset specific code to find the MP
 *	   Floating Pointer structure.
 **/

#ifdef CONFIG_ARCH_x86_32
	#if  __SCALING__ >= SCALING_SMP
void *chipset_findx86MpFp(void);
	#endif
#endif

#ifdef CHIPSET_HAS_ACPI
void *chipset_findAcpiRsdp(void);
#endif

#endif

