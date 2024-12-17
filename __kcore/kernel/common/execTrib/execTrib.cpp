
#include <debug.h>
#include <arch/arch.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kclasses/cachePool.h>
#include <__kstdlib/__kclib/string.h>
#include <kernel/common/execTrib/execTrib.h>


ExecTrib::ExecTrib(void)
{
	memset(parsers, 0, sizeof(*parsers) * EXECTRIB_MAX_NPARSERS);
}

error_t ExecTrib::initialize(void)
{
	error_t		ret;

	ret = (*elfParser.initialize)(ARCH_SHORT_STRING, __VADDR_NBITS__);
	if (ret != ERROR_SUCCESS) {
		return ret;
	};

	parsers[0].desc = &elfParser;
	FLAG_SET(parsers[0].flags, EXECTRIB_PARSER_FLAGS_STATIC);
	return ERROR_SUCCESS;
}

sExecutableParser *ExecTrib::identify(void *buff)
{
	for (uarch_t i=0; i<EXECTRIB_MAX_NPARSERS; i++)
	{
		if (parsers[i].desc == NULL) {
			return NULL;
		};

		if ((*parsers[i].desc->identify)(buff)) {
			return parsers[i].desc;
		};
	};

	return NULL;
}

