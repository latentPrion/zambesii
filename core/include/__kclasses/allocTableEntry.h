#ifndef _ALLOC_TABLE_ENTRY_H
	#define _ALLOC_TABLE_ENTRY_H

	#include <scaling.h>
	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/numaTypes.h>

/**	EXPLANATION:
 * An abstract class to hold information on page granular dynamic memory
 * allocations. It holds the virtual address and size (in pages) of the
 * allocation. Within the 'nPages' member is also a set of attribute values
 * to tell the swapper what *not* to swap out to disk, and other optimization
 * information.
 **/
class allocTableEntryC
{
public:
	void		*vaddr;
	uarch_t		nPages;

public:
	inline sarch_t operator ==(allocTableEntryC &ate);
	inline sarch_t operator ==(int v);
	inline sarch_t operator !=(int v);
	inline sarch_t operator >(allocTableEntryC &ate);
	inline sarch_t operator <(allocTableEntryC &ate);
	inline sarch_t operator >=(allocTableEntryC &ate);
	inline sarch_t operator <=(allocTableEntryC &ate);
	// inline allocTableEntryC &operator =(allocTableEntryC &ate);
	inline allocTableEntryC &operator =(int v);
};


/**	Inline Methods
 ***************************************************************************/

sarch_t allocTableEntryC::operator ==(allocTableEntryC &ate)
{
	return vaddr == ate.vaddr;
}

sarch_t allocTableEntryC::operator ==(int v)
{
	if (v == 0) {
		return vaddr == NULL;
	};
	return 0;
}

sarch_t allocTableEntryC::operator !=(int v)
{
	if (v == 0) {
		return vaddr != NULL;
	};
	return 0;
}

sarch_t allocTableEntryC::operator >(allocTableEntryC &ate)
{
	return vaddr > ate.vaddr;
}

sarch_t allocTableEntryC::operator <(allocTableEntryC &ate)
{
	return vaddr < ate.vaddr;
}

sarch_t allocTableEntryC::operator >=(allocTableEntryC &ate)
{
	return vaddr >= ate.vaddr;
}

sarch_t allocTableEntryC::operator <=(allocTableEntryC &ate)
{
	return vaddr <= ate.vaddr;
}

// allocTableEntryC &allocTableEntryC::operator =(allocTableEntryC &ate);

allocTableEntryC &allocTableEntryC::operator =(int v)
{
	if (v == 0) {
		vaddr = 0;
	};
	return *this;
}

#endif

