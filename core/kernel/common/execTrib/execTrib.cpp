
#include <arch/arch.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/string.h>
#include <kernel/common/execTrib/execTrib.h>


execTribC::execTribC(void)
{
	memset(parsers, 0, sizeof(executableParserS) * EXECTRIB_MAX_NPARSERS);
}

error_t execTribC::initialize(void)
{
	error_t		ret;

	ret = (*elfParser.initialize)(ARCH_SHORT_STRING, __WORDSIZE);
	if (ret != ERROR_SUCCESS) {
		return ret;
	};

	parsers[0].desc = &elfParser;
	__KFLAG_SET(parsers[0].flags, EXECTRIB_PARSER_FLAGS_STATIC);
	return ERROR_SUCCESS;
}

executableParserS *execTribC::identify(void *buff)
{
	for (uarch_t i=0; i<EXECTRIB_MAX_NPARSERS; i++)
	{
		if (parsers[i].desc == __KNULL) {
			return __KNULL;
		};

		if ((*parsers[i].desc->identify)(buff)) {
			return parsers[i].desc;
		};
	};

	return __KNULL;
}

