#ifndef _ALLOC_TABLE_ENTRY_H
	#define _ALLOC_TABLE_ENTRY_H

	#include <scaling.h>
	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/numaTypes.h>

/**	EXPLANATION:
 * A class to hold generic allocation information about a range of memory in a
 * process's address space: The TYPE and FLAGS fields are part of the 'nPages'
 * field.
 *
 * There is a caveat here: the TYPE and FLAGS field take up 3 bits and 5 bits
 * respectively. That is, they take up 8 bits of the 'nPages' field, no matter
 * which arch is being compiled for. That is, on an arch with 128-byte or less
 * granular pages, we would have a bug problem. Luckily, no arch vendors have
 * 32+ bit CPUs with 128 byte page sizes.
 *
 *	NOTE:
 * Remember never to try to pack values into the 'vaddr' member. This member
 * must be left unadulterated since the comparison operators for this class
 * rely on it. Keep things simple. In other words, this class has been packed
 * as densely as it can be.
 **/

#define ALLOCTABLE_TYPE_CODE		0x0
#define ALLOCTABLE_TYPE_DATA		0x1
#define ALLOCTABLE_TYPE_RODATA		0x2
#define ALLOCTABLE_TYPE_STACK		0x3
#define ALLOCTABLE_TYPE_NOSWAP		0x4
#define ALLOCTABLE_TYPE_SHLIB		0x5
#define ALLOCTABLE_TYPE_SHMEM		0x6

#define ALLOCTABLE_FLAGS_CONTIG		(1<<1)
#define ALLOCTABLE_FLAGS_FRAGMENTED	0

#define ALLOCTABLE_NPAGES_SHIFT		(8)

#define ALLOCTABLE_TYPE_SHIFT		(5)
#define ALLOCTABLE_TYPE_MASK		(0x7)

#define ALLOCTABLE_FLAGS_MASK		(0x1F)

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
		return vaddr == __KNULL;
	};
	return 0;
}

sarch_t allocTableEntryC::operator !=(int v)
{
	if (v == 0) {
		return vaddr != __KNULL;
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

