#ifndef _EXECUTABLE_LINKABLE_FORMAT_H
	#define _EXECUTABLE_LINKABLE_FORMAT_H

	#include <__kstdlib/__ktypes.h>

/**	EXPLANATION:
 * Base structure definitions for ELF types. Note well that we don't use those
 * ugly Elf_*_Half, and "Word" and "Off" type names from the spec, of course.
 **/

#define EHDR_TYPE_NONE		(0)
#define EHDR_TYPE_RELOCATABLE	(1)
#define EHDR_TYPE_EXECUTABLE	(2)
#define EHDR_TYPE_DYNAMIC	(3)
#define EHDR_TYPE_CORE		(4)
#define EHDR_TYPE_LOPROC	(0xFF00)
#define EHDR_TYPE_HIPROC	(0xFFFF)

#define EHDR_ARCH_NONE		(0)
#define EHDR_ARCH_M32		(0x1)
#define EHDR_ARCH_SPARC32	(0x2)
#define EHDR_ARCH_i386		(0x3)
#define EHDR_ARCH_M68K		(0x4)
#define EHDR_ARCH_M88K		(0x5)
#define EHDR_ARCH_i860		(0x7)
#define EHDR_ARCH_MIPSR3K	(0x8)

#define EHDR_VERSION_NONE	(0x0)
#define EHDR_VERSION_CURRENT	(0x1)

#define EHDR_ID_MAGIC		"\x7F""ELF"

#define EHDR_ID_CLASS_IDX	0x4
#define EHDR_ID_CLASS_NONE	0
#define EHDR_ID_CLASS_32	1
#define EHDR_ID_CLASS_64	2

#define EHDR_ID_DATA_IDX	0x5
#define EHDR_ID_DATA_NONE	0
#define EHDR_ID_DATA_LE		1
#define EHDR_ID_DATA_BE		2

#define EHDR_ID_VERSION_IDX	0x6
#define EHDR_ID_PAD		0x7

struct sElfHeader
{
	ubit8		ident[16];
	ubit16		type;
	ubit16		arch;
	ubit32		version;
	ubit32		entryPoint;
	ubit32		phdrOffset;
	ubit32		shdrOffset;
	ubit32		flags;

	ubit16		ehdrSize;
	ubit16		phdrEntrySize;
	ubit16		phdrNEntries;
	ubit16		shdrEntrySize;
	ubit16		shdrNEntries;
	ubit16		shdrStrTableIndex;
};

void elfheader_dump(struct sElfHeader *ehdr);


#define ESHDR_IDX_UNDEF		(0)
#define ESHDR_IDX_LORESERVE	(0xFF00)
#define ESHDR_IDX_LOPROC	(0xFF00)
#define ESHDR_IDX_HIPROC	(0xFF1F)
#define ESHDR_IDX_ABS		(0xFFF1)
#define ESHDR_IDX_COMMON	(0xFFF2)
#define ESHDR_IDX_HIRESERVE	(0xFFFF)


#define ESHDR_TYPE_NULL		0
#define ESHDR_TYPE_PROGBITS	1
#define ESHDR_TYPE_SYMTAB	2
#define ESHDR_TYPE_STRTAB	3
#define ESHDR_TYPE_RELA		4
#define ESHDR_TYPE_HASH		5
#define ESHDR_TYPE_DYNAMIC	6
#define ESHDR_TYPE_NOTE		7
#define ESHDR_TYPE_NOBITS	8
#define ESHDR_TYPE_REL		9
#define ESHDR_TYPE_SHLIB	10
#define ESHDR_TYPE_DYNSYM	11
#define ESHDR_TYPE_LOPROC	0x70000000
#define ESHDR_TYPE_HIPROC	0x7FFFFFFF
#define ESHDR_TYPE_LOUSER	0x80000000
#define ESHDR_TYPE_HIUSER	0xFFFFFFFF

#define ESHDR_FLAGS_WRITE	(1<<0)
#define ESHDR_FLAGS_ALLOC	(1<<1)
#define ESHDR_FLAGS_EXECINSTR	(1<<2)
#define ESHDR_FLAGS_PROC_MASK	0xF0000000

struct elf32_shdrEntryS
{
	// Index of name in string table.
	ubit32		name;
	ubit32		type;
	ubit32		flags;
	ubit32		vaddr;
	ubit32		offset;
	ubit32		size;
	ubit32		link;
	ubit32		info;
	ubit32		align;
	ubit32		entrySize;
};


#define ESYMTAB_INFO_SET(b,t)		(((b) << 4) + ((t) & 0xF))
#define ESYMTAB_INFO_GET_BIND(i)	((i) >> 4)
#define ESYMTAB_INFO_BIND_LOCAL		0
#define ESYMTAB_INFO_BIND_GLOBAL	1
#define ESYMTAB_INFO_BIND_WEAK		2
#define ESYMTAB_INFO_BIND_LOPROC	13
#define ESYMTAB_INFO_BIND_HIPROC	15

#define ESYMTAB_INFO_GET_TYPE(i)	((i) & 0xF)
#define ESYMTAB_INFO_TYPE_NONE		0
#define ESYMTAB_INFO_TYPE_OBJECT	1
#define ESYMTAB_INFO_TYPE_FUNCTION	2
#define ESYMTAB_INFO_TYPE_SECTION	3
#define ESYMTAB_INFO_TYPE_FILE		4
#define ESYMTAB_INFO_TYPE_LOPROC	13
#define ESYMTAB_INFO_TYPE_HIPROC	15

struct elf32_symTabEntryS
{
	ubit32		name;
	ubit32		value;
	ubit32		size;
	ubit8		info;
	ubit8		other;
	ubit16		section;
};

// This describes the machine on which the parser has been loaded.
struct sElfModuleState
{
	// String passed by the kernel on initialize().
	utf8Char	*__karchString;
	// Word size passed by kernel on initialize().
	ubit8		__kwordSize;
	// Correct value for ELF "machine" for this build.
	sbit32		elfArch;
	// Correct value for ELF "data" (2MSB/2LSB) on this build.
	ubit8		elfEndianness;
	ubit32		elfWordSize;
};

extern struct sElfModuleState	elfModuleState;

#endif

