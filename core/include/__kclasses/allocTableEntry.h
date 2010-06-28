#ifndef _ALLOC_TABLE_ENTRY_H
	#define _ALLOC_TABLE_ENTRY_H

	#include <scaling.h>
	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/numaTypes.h>

#define ALLOCTABLE_FLAGS_CONTIG		(1<<2)
#define ALLOCTABLE_FLAGS_FRAGMENTED	0

#define ALLOCTABLE_TYPE_DATA		0x1
#define ALLOCTABLE_TYPE_RODATA		0x2
#define ALLOCTABLE_TYPE_EXEC		0x3
#define ALLOCTABLE_TYPE_NOSWAP		0x4

class allocTableEntryC
{
public:
	void		*vaddr;
	uarch_t		nPages;
	ubit8		type, flags;

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
		return vaddr == __KNULL;
	};
	return 0;
}

sarch_t allocTableEntryC::operator !=(int v)
{
	if (v == 0) {
		return vaddr == __KNULL;
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

