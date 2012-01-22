
#include <__kstdlib/__kclib/string8.h>
#include <kernel/common/execTrib/elf.h>

error_t elf_initialize(utf8Char *archString, ubit16 wordSize)
{
	error_t		ret=ERROR_CRITICAL;

	/* The kernel will pass only the arch string and the word size.
	 * The lib is expected to derive the local arch's values for the ELF
	 * "machine", "data" (endianness) and "class".
	 **/
	elfModuleState.__karchString = archString;
	elfModuleState.__kwordSize = wordSize;

	// Set all values to their "none" states as default.
	elfModuleState.elfArch = EHDR_ARCH_NONE;
	elfModuleState.elfEndianness = EHDR_ID_DATA_NONE;
	elfModuleState.elfWordSize = EHDR_ID_CLASS_NONE;

	if (strcmp8(archString, CC"x86-32") == 0)
	{
		elfModuleState.elfArch = EHDR_ARCH_i386;
		elfModuleState.elfEndianness = EHDR_ID_DATA_LE;
		elfModuleState.elfWordSize = EHDR_ID_CLASS_32;
		ret = ERROR_SUCCESS;
	};
	if (strcmp8(archString, CC"x86-64") == 0)
	{
		// FIXME: Find out what ELF uses for x86-64.
		// elfModuleState.elfMachine = EHDR_ARCH_???;
		elfModuleState.elfEndianness = EHDR_ID_DATA_LE;
		elfModuleState.elfWordSize = EHDR_ID_CLASS_64;
		ret = ERROR_SUCCESS;
	};

	return ret;
}

sarch_t elf_identify(void *buff)
{
	struct elfHeaderS	*ehdr;

	ehdr = (struct elfHeaderS *)buff;
	if (strncmp8((char *)ehdr->ident, EHDR_ID_MAGIC, 4) == 0) {
		return 1;
	};
	return 0;
}

sarch_t elf_isLocalArch(void *buff)
{
	struct elfHeaderS	*ehdr;

	ehdr = (struct elfHeaderS *)buff;
	if (ehdr->arch == elfModuleState.elfArch
		&& ehdr->ident[EHDR_ID_CLASS_IDX] == elfModuleState.elfWordSize
		&& ehdr->ident[EHDR_ID_DATA_IDX]
			== elfModuleState.elfEndianness)
	{
		return 1;
	};

	return 0;
}

