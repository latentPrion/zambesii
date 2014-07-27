
#ifndef _MULTIBOOT_H
	#define _MULTIBOOT_H

	#include <__kstdlib/__ktypes.h>

#define MULTIBOOT_HEADER_MAGIC		(0x1BADB002)
#define MULTIBOOT_DATA_MAGIC		(0x2BADB002)

//We don't use modules, but we set this anyway.
#define MULTIBOOT_HEADER_NEED_MODULES_ALIGNED			0x1u
#define MULTIBOOT_HEADER_NEED_MEMORY_MAP			0x2u
#define MULTIBOOT_HEADER_NEED_VIDEO_MODE_ASSIST			0x4u
#define MULTIBOOT_HEADER_MANUAL_EXECUTABLE_SECTIONS		0x10000u

#ifndef __ASM__

struct sMultibootHeader
{
	//Must be MULTIBOOT_MAGIC - see above.
	ubit32 magic;
	//Feature flags.
	ubit32 flags;
	//The above fields plus this one must equal 0 mod 2^32.
	ubit32 checksum;
	//These are only valid if MULTIBOOT_AOUT_KLUDGE is set.
	ubit32 header_addr;
	ubit32 load_addr;
	ubit32 load_end_addr;
	ubit32 bss_end_addr;
	ubit32 entry_addr;
	//These are only valid if MULTIBOOT_VIDEO_MODE is set.
	ubit32 mode_type;
	ubit32 width;
	ubit32 height;
	ubit32 depth;
};

//The symbol table for a.out.
struct sAoutSymbolTable
{
	ubit32	tabsize;
	ubit32	strsize;
	ubit32	addr;
	ubit32	reserved;
};

//The section header table for ELF.
struct sElfSectionHeaderTable
{
	ubit32	num;
	ubit32	size;
	ubit32	addr;
	ubit32	shndx;
};

//This is the structure of the data the Multiboot Bootloader passes to the OS.
struct sMultibootData
{
	ubit32 flags;
	ubit32	mem_lower;
	ubit32	mem_upper;
	ubit32	boot_device;
	ubit8	*cmdline;
	ubit32	mods_count;
	ubit32	mods_addr;
	union
	{
		sAoutSymbolTable aout_sym;
		sElfSectionHeaderTable elf_sec;
	} u;
	ubit32	mmap_length;
	ubit32	mmap_addr;
};

// The module structure.
struct Module
{
	ubit32	mod_start;
	ubit32	mod_end;
	ubit32	string;
	ubit32	reserved;
};

/* The memory map. Be careful that the offset 0 is base_addr_low
 * but no size.
 **/
struct sMemoryMap
{
	ubit32	size;
	ubit32	base_addr_low;
	ubit32	base_addr_high;
	ubit32	length_low;
	ubit32	length_high;
	ubit32	type;
};

#endif /* !defined( __ASM__ ) */

#endif
