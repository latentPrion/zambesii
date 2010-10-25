
#include <__kstdlib/__kclib/string.h>
#include <kernel/common/execTrib/elf.h>

error_t elf_identify(void *buff)
{
	elfHeaderS	*ehdr;

	ehdr = reinterpret_cast<elfHeaderS *>( buff );
	if (strncmp(reinterpret_cast<char *>( ehdr->ident ), EHDR_ID_MAGIC, 4)
		== 0)
	{
		return 1;
	};
	return 0;
}

