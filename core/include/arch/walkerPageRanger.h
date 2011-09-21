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

/**	IMPORTANT:
 * When you call walkerPageRanger::map*(), the virtual address you map whatever
 * physical address you wanted to, must be adjusted. Consider the following:
 *
 * You have allocated a 2 page range of virtual memory to play with. The
 * VMM would only ever return page aligned virtual addresses, so you have an
 * address of the form, 0xXXXXX000 on an arch with 4KB page sizes.
 *
 * You wish to map this virtual memory to the physical address 0xABCDE012. The
 * physical address is NOT page aligned, and when you map the virtual memory to
 * that physical address, you'd have now mapped you virtual 0xXXXXX000 to
 * physical 0xABCDE000, and NOT to 0xABCDE012. When you dereference your pointer
 * now, you'll write to the wrong place unless you add the offset within the
 * physical memory range to your virtual pointer.
 *
 * The following macro, WPRANGER_ADJUST_VADDR() does just that. Use like this:
 *	vaddr = WPRANGER_ADJUST_VADDR(
 *		VIRTUAL_ADDRESS_POINTER,
 *		PHYSICAL_ADDRESS_YOU_WILL_MAP_TO,
 *		TYPE_OF_THE_VADDR_POINTER_ARGUMENT);
 *
 * For example:
 *	struct mystruct		*s;
 *
 *	s = (memoryTrib.__kmemoryStream.vaddrSpaceStream
 *		.*memoryTrib.__kmemoryStream.vaddrSpaceStream.getPages)(1);
 *
 *	// The address returned by the VMM is page aligned.
 *
 *	// Map 's' to 0xFCFCF012 in physical memory:
 *	walkerPageRanger::mapInc(
 *		SOME_VADDRSPACE,
 *		s, 0xFCFCF012,
 *		HOWEVER_MANY_PAGES_YOU_NEED_TO_MAP,
 *		PERMISSION | FLAGS | AS | NEEDED);
 *
 * After that call to mapInc(), the virtual address in 's' is still the page
 * aligned address returned by the VMM. You must of course add the offset into
 * the page to get the right data access. The following would yield an errant
 * write until the pointer is adjusted:
 *	s->field = FOOVAL;
 *
 * We have just trampled on something we weren't supposed to do. So we make sure
 * to adjust the pointer after mapping before using it:
 *	s = WPRANGER_ADJUST_VADDR(s, 0xFCFCF012, struct mystruct *);
 *
 * TL;DR: After you use mapInc(), remapInc(), remapNoInc() or mapNoInc(), on a
 * raw page allocated from the VMM, be sure to call WPRANGER_ADJUST_VADDR()
 * before using the newly mapped pointer.
 *
 * --Thanks.
 **/
#define WPRANGER_ADJUST_VADDR(_v,_p,_t)			\
	(reinterpret_cast<_t>( \
		(((uarch_t)(_v)) & PAGING_BASE_MASK_HIGH) \
		+ ((_p) & PAGING_BASE_MASK_LOW) ))


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
		vaddrSpace, vaddr,
		(PAGESTATUS_SWAPPED << PAGING_PAGESTATUS_SHIFT), nPages,
		WPRANGER_OP_CLEAR_PRESENT, 0);
}

#endif

