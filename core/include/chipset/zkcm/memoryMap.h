#ifndef _CHIPSET_MEMORY_MAP_H
	#define _CHIPSET_MEMORY_MAP_H

	#include <arch/paddr_t.h>
	#include <__kstdlib/__ktypes.h>

/**	EXPLANATION:
 * This compile-time interface is used to allow a person porting a chipset to
 * compile-in a memory map of physical RAM on his/her particular chipset.
 *
 * If the pointer at the end of this file is not NULL, the kernel will assume
 * that the information given in this interface overrides anything the firmware
 * can provide; Technically the assumption is that the firmware is /unable/ to
 * provide a memory map, so the person porting the chipset, having knowledge of
 * the chipset's memory configuration, was nice prudent enough to provide his
 * /her own.
 *
 * The Firmware Trib must return a struct of this form as well, take note.
 **/

#define ZKCM_MMAP_TYPE_USABLE			0
#define ZKCM_MMAP_TYPE_RECLAIMABLE		1
#define ZKCM_MMAP_TYPE_RESERVED			2
#define ZKCM_MMAP_TYPE_MMIO			3
#define ZKCM_MMAP_TYPE_FIRMWARE			4

struct zkcmMemMapEntryS
{
	paddr_t		baseAddr, size;
	// Must be signed so it can hold the values defined above.
	sbit8		memType;
};

struct zkcmMemMapS
{
	struct zkcmMemMapEntryS		*entries;
	ubit32				nEntries;
};

#endif

