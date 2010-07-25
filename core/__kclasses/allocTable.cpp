
#include <__kclasses/allocTable.h>

error_t allocTableC::addEntry(
	void *vaddr, uarch_t nPages, ubit8 type, ubit8 flags
	)
{
	allocTableEntryC	*result;
	allocTableEntryC	tmp =
	{
		vaddr, nPages, type, flags
	};

	result = allocTable.addEntry(&tmp);
	if (result == __KNULL) {
		return ERROR_MEMORY_NOMEM;
	};
	return ERROR_SUCCESS;
}

void allocTableC::removeEntry(void *vaddr)
{
	allocTableEntryC	tmp =
	{
		vaddr, 0, 0, 0
	};

	allocTable.removeEntry(&tmp);
}

error_t allocTableC::lookup(
	void *vaddr, uarch_t *nPages, ubit8 *type, ubit8 *flags
	)
{
	if (nPages == __KNULL || type == __KNULL || flags == __KNULL) {
		return ERROR_INVALID_ARG;
	};

	allocTableEntryC	tmp =
	{
		vaddr, 0, 0, 0
	};
	allocTableEntryC	*ret;

	ret = allocTable.find(&tmp);
	if (ret == __KNULL) {
		return ERROR_GENERAL;
	};

	*nPages = ret->nPages;
	*type = ret->type;
	*flags = ret->flags;

	return ERROR_SUCCESS;
}

