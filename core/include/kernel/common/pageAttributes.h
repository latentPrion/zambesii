#ifndef _PAGE_ATTRIBUTES_H
	#define _PAGE_ATTRIBUTES_H

#define PAGEATTRIB_WRITE			(1<<1)
#define PAGEATTRIB_SUPERVISOR			(1<<2)
#define PAGEATTRIB_EXECUTABLE			(1<<3)
#define PAGEATTRIB_PRESENT			(1<<4)
#define PAGEATTRIB_ACCESSED			(1<<5)
#define PAGEATTRIB_DIRTY			(1<<6)
#define PAGEATTRIB_TLB_GLOBAL			(1<<7)
#define PAGEATTRIB_CACHE_DISABLED		(1<<8)
#define PAGEATTRIB_CACHE_WRITE_THROUGH		(1<<9)

/*	NOTE:
 * This is more of a Walker Page Ranger option when mapping, used only for
 * certain special use cases in the kernel; You most likely don't need to use
 * this flag. It is defined here solely for ease of maintenance since all the
 * other values for the "flags" argument to Walker Page Ranger are in here too.
 * In theory it should be in walkerPageRanger.h and should be called
 * WPRANGER_FLAG_FLUSH_LOCALLY_ONLY since it is a WPR option, and not a page
 * attribute.
 *
 * This flag when passed, is interpreted by Walker Page Ranger directly, and
 * does not affect the way the relevant page is mapped. It only indicates to
 * WPR that it should not attempt to do a TLB flush on any CPU other than the
 * one on which the page is being mapped, even if the address space being
 * modified is currently loaded on another CPU.
 *
 * In effect, it is a force flag to ensure that an address space change is only
 * properly handled on ONE CPU. This is undesirable in most cases, and again,
 * you most likely do not need to use this flag.
 **/
#define PAGEATTRIB_LOCAL_FLUSH_ONLY		(1<<15)

/**	EXPLANATION:
 * These next few values are for the page faulting code to quickly handle a page
 * fault by telling the page fault code exactly what type of page has been
 * faulted on, and what to do with it.
 *
 * *NONE* of these flags can be 0x0. This is because the only way the pagefault
 * code can know that a page is not unmapped is if the page table entry is not
 * equal to 0.
 **/
#define PAGESTATUS_SWAPPED		(0x1)
#define PAGESTATUS_GUARDPAGE		(0x2)
#define PAGESTATUS_FAKEMAPPED_STATIC	(0x3)
#define PAGESTATUS_FAKEMAPPED_DYNAMIC	(0x4)

#define PAGESTATUS_MASK			(0x7)

/**	EXPLANATION:
 * For a FAKEMAPPED_STATIC page, the kernel will use these extra 'type' bits
 * to determine what type of data must be demand paged into this page range on
 * a translation (page) fault.
 **/
#define PAGETYPE_SHLIB			(0x0)
#define PAGETYPE_EXEC_SECTION		(0x1)
#define PAGETYPE_MMAPPED_FILE		(0x2)

#define PAGETYPE_MASK			(0x7)

#endif

