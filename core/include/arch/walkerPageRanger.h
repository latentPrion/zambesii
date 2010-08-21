#ifndef _WALKER_PAGE_RANGER_H
	#define _WALKER_PAGE_RANGER_H

	#include <arch/paging.h>
	#include <arch/paddr_t.h>
	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/vaddrSpace.h>
	#include <kernel/common/pageAttributes.h>

// These are the valid return values for walkerPageRanger::lookup() and unmap().
#define WPRANGER_STATUS_BACKED			(0)
#define WPRANGER_STATUS_SWAPPED			(1)
#define WPRANGER_STATUS_FAKEMAPPED_STATIC	(2)
#define WPRANGER_STATUS_FAKEMAPPED_DYNAMIC	(3)
#define WPRANGER_STATUS_GUARDPAGE		(4)
#define WPRANGER_STATUS_UNMAPPED		(5)

// Generic operations on attributes.
#define WPRANGER_OP_SET			(1)
#define WPRANGER_OP_CLEAR		(2)
// This is only valid for setAttributes(). Not remap*().
#define WPRANGER_OP_ASSIGN		(3)
// More specific, customized operations for special cases, like COW, etc.
#define WPRANGER_OP_SET_PRESENT		(4)
#define WPRANGER_OP_CLEAR_PRESENT	(5)
#define WPRANGER_OP_SET_WRITE		(6)
#define WPRANGER_OP_CLEAR_WRITE		(7)

namespace walkerPageRanger
{
	void swapMap(
		vaddrSpaceC *vaddrSpace,
		void *vaddr, uarch_t nPages);

	/**	EXPLANATION:
	 * This next group of functions are the back-ends for all of the inline
	 * functions declared above.
	 *
	 * They each have their own caveats, so please read up on them before
	 * using them.
	 **/
	status_t mapInc(
		vaddrSpaceC *vaddrSpace,
		void *vaddr, paddr_t paddr, uarch_t nPages, uarch_t flags);

	status_t mapNoInc(
		vaddrSpaceC *vaddrSpace,
		void *vaddr, paddr_t paddr, uarch_t nPages, uarch_t flags);

	void remapInc(
		vaddrSpaceC *vaddrSpace,
		void *vaddr, paddr_t paddr, uarch_t nPages,
		ubit8 op, uarch_t flags);

	void remapNoInc(
		vaddrSpaceC *vaddrSpace,
		void *vaddr, paddr_t paddr, uarch_t nPages,
		ubit8 op, uarch_t flags);

	void setAttributes(
		vaddrSpaceC *vaddrSpace,
		void *vaddr, uarch_t nPages,
		ubit8 op, uarch_t flags);

	status_t lookup(
		vaddrSpaceC *vaddrSpace,
		void *vaddr, paddr_t *paddr, uarch_t *flags);

	status_t unmap(
		vaddrSpaceC *vaddrSpace,
		void *vaddr, paddr_t *paddr, uarch_t nPages, uarch_t *flags);

	// Architecture specific kernel flag converter.
	uarch_t decodeFlags(uarch_t flags);
	uarch_t encodeFlags(uarch_t flags);	
}


/**	Inline Methods
 *************************************************************************/

inline void walkerPageRanger::swapMap(
	vaddrSpaceC *vaddrSpace,
	void *vaddr, uarch_t nPages
	)
{
	// Map all pages as swapped and clear the present attributes.
	remapNoInc(
		vaddrSpace, vaddr, PAGING_LEAF_SWAPPED, nPages,
		WPRANGER_OP_CLEAR_PRESENT, 0);
}

#endif

