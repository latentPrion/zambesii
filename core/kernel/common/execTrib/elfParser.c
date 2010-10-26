
#include <kernel/common/execTrib/elf.h>
#include <kernel/common/execTrib/executableFormat.h>
#include "elfFuncs.h"


struct elfModuleStateS		elfModuleState;

struct executableParserS	elfParser =
{
	&elf_initialize,
	&elf_identify,
	&elf_isLocalArch
};

