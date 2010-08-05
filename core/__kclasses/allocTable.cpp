
#include <__kclasses/allocTable.h>

error_t allocTableC::addEntry(
	void *vaddr, uarch_t nPages, ubit8 type, ubit8 flags
	)
{
	allocTableEntryC	*result;
	allocTableEntryC	tmp;

	tmp.vaddr = vaddr;

	// Pack the type and flags attributes into the nPages member.
	tmp.nPages = (nPages << ALLOCTABLE_NPAGES_SHIFT)
		| ((type & ALLOCTABLE_TYPE_MASK) << ALLOCTABLE_TYPE_SHIFT)
		| (flags & ALLOCTABLE_FLAGS_MASK);

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
		vaddr, 0
	};

	allocTable.removeEntry(&tmp);
}

/**	TODO:
 * lookup() and removeEntry() can be optimized by having lookup() return the
 * direct address of the item that was searched for. Change lookup to return
 * the address that the real internal allocTable returns, so that the calling
 * function can then pass the direct address to removeEntry.
 *
 * This would require an implementation of removeEntry in __kequalizerListC<>
 * that can remove an entry by its address directly. Doing this would *greatly*
 * speed up deallocations from alloc table enabled allocation methods.
 **/
error_t allocTableC::lookup(
	void *vaddr, uarch_t *nPages, ubit8 *type, ubit8 *flags
	)
{
	if (nPages == __KNULL || type == __KNULL || flags == __KNULL) {
		return ERROR_INVALID_ARG;
	};

	allocTableEntryC	tmp =
	{
		vaddr, 0
	};
	allocTableEntryC	*ret;

	ret = allocTable.find(&tmp);
	if (ret == __KNULL) {
		return ERROR_GENERAL;
	};

	// Unpack the nPages field.
	*nPages = (ret->nPages >> ALLOCTABLE_NPAGES_SHIFT);
	*type = (ret->nPages >> ALLOCTABLE_TYPE_SHIFT) & ALLOCTABLE_TYPE_MASK;
	*flags = ret->nPages & ALLOCTABLE_FLAGS_MASK;

	return ERROR_SUCCESS;
}

