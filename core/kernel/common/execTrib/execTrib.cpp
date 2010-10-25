
#include <__kstdlib/__kclib/string.h>
#include <kernel/common/execTrib/execTrib.h>


execTribC::execTribC(void)
{
	memset(parsers, 0, sizeof(executableFormatS) * EXECTRIB_MAX_NPARSERS);
}

