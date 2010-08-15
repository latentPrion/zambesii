#ifndef _PAGING_H
	#define _PAGING_H
	//x86-32

	#include <arch/x8632/paddr_t.h>
	#include <__kstdlib/__ktypes.h>

	/**	EXPLANATION:
	 * On x86-32, we have to cater for a build with the PAE enabled, so this
	 * paging.h is a bit more complex, probably.
	 **/

#ifdef CONFIG_ARCH_x86_32_PAE
	#define PAGING_NLEVELS			(3)
	#define PAGING_L0_NENTRIES		(4)
	#define PAGING_L1_NENTRIES		(512)
	#define PAGING_L2_NENTRIES		(512)

	#define PAGING_L0_MAPSIZE		(0x40000000)
	#define PAGING_L1_MAPSIZE		(0x00200000)
	#define PAGING_L2_MAPSIZE		(0x00001000)
#else
	#define PAGING_NLEVELS			(2)
	#define PAGING_L0_NENTRIES		(1024)
	#define PAGING_L1_NENTRIES		(1024)

	#define PAGING_L0_MAPSIZE		(0x00400000)
	#define PAGING_L1_MAPSIZE		(0x00001000)
#endif

#define PAGING_BASE_SIZE			(0x00001000)
#ifdef CONFIG_ARCH_x86_32_PAE
	#define PAGING_ALTERNATE_SIZE		(0x00200000)
#else
	// #define PAGING_ALTERNATE_SIZE	0x00400000
#endif

#define PAGING_BASE_MASK_LOW		(PAGING_BASE_SIZE - 1)
#define PAGING_BASE_MASK_HIGH		~(PAGING_BASE_MASK_LOW)
#define PAGING_ALTERNATE_MASK_LOW	(PAGING_ALTERNATE_SIZE - 1)
#define PAGING_ALTERNATE_MASK_HIGH	~(PAGING_ALTERNATE_MASK_LOW)

/* Define the mask and shift offsets to allow the kernel to extract the page
 * table entry number from any level of the paging hierarchy.
 **/
#ifdef CONFIG_ARCH_x86_32_PAE
	#define PAGING_L0_VADDR_MASK			((uarch_t)0xC0000000)
	#define PAGING_L1_VADDR_MASK			((uarch_t)0x3FE00000)
	#define PAGING_L2_VADDR_MASK			((uarch_t)0x001FF000)

	#define PAGING_L0_VADDR_SHIFT			(30)
	#define PAGING_L1_VADDR_SHIFT			(21)
	#define PAGING_L2_VADDR_SHIFT			(12)
#else
	#define PAGING_L0_VADDR_MASK			((uarch_t)0xFFC00000)
	#define PAGING_L1_VADDR_MASK			((uarch_t)0x003FF000)

	#define PAGING_L0_VADDR_SHIFT			(22)
	#define PAGING_L1_VADDR_SHIFT			(12)
#endif

// Macros to extract and encode a paddr into an L<N> entry.
#define PAGING_L0_ENCODE_PADDR(__p)	(__p)
#define PAGING_L0_EXTRACT_PADDR(__p)	((__p) & (~(paddr_t)0xFFF))

#ifdef CONFIG_ARCH_x86_32_PAE
	#define PAGING_L1_ENCODE_PADDR(__p)	(__p)
	#define PAGING_L1_EXTRACT_PADDR(__p)	((__p) & (~(paddr_t)0xFFF))
#endif

#define PAGING_LEAF_ENCODE_PADDR(__p)	(__p)
#define PAGING_LEAF_EXTRACT_PADDR(__p)	((__p) & (~(paddr_t)0xFFF))

#ifdef CONFIG_ARCH_x86_32_PAE
	// Level0 PAE Arch Specific Flags.
	#define PAGING_L0_PRESENT		(1<<0)
	#define PAGING_L0_CACHE_WRITE_THROUGH	(1<<3)
	#define PAGING_L0_CACHE_DISABLED	(1<<4)
#else
	// Level0 no-PAE Arch Specific Flags.
	#define PAGING_L0_PRESENT		(1<<0)
	#define PAGING_L0_WRITE			(1<<1)
	#define PAGING_L0_USER			(1<<2)
	#define PAGING_L0_CACHE_WRITE_THROUGH	(1<<3)
	#define PAGING_L0_CACHE_DISABLED	(1<<4)
	#define PAGING_L0_TLB_GLOBAL		(1<<8)
#endif

// Level1 Arch Specific Flags.
#define PAGING_L1_PRESENT		(1<<0)
#define PAGING_L1_WRITE			(1<<1)
#define PAGING_L1_USER			(1<<2)
#define PAGING_L1_CACHE_WRITE_THROUGH	(1<<3)
#define PAGING_L1_CACHE_DISABLED	(1<<4)
#define PAGING_L1_TLB_GLOBAL		(1<<8)
#ifdef CONFIG_ARCH_x86_32_PAE
	#define PAGING_L1_NO_EXECUTE	(1<<63)
#endif

#ifdef CONFIG_ARCH_x86_32_PAE
	// Level2 PAE Arch Specific Flags.
	#define PAGING_L2_PRESENT		(1<<0)
	#define PAGING_L2_WRITE			(1<<1)
	#define PAGING_L2_USER			(1<<2)
	#define PAGING_L2_CACHE_WRITE_THROUGH	(1<<3)
	#define PAGING_L2_CACHE_DISABLED	(1<<4)
	#define PAGING_L2_TLB_GLOBAL		(1<<8)
	#define PAGING_L2_NO_EXECUTE		(1<<63)
#endif

/* Though these next two flags may seem insignificant, they are in fact very
 * important: they are the architecture specific flags which map a page as
 * swapped or fakemapped.
 *
 * These are used as a bitfield in an unbacked entry which has attributes, but
 * no physical backing memory. When the kernel faults, it will test these to
 * determine what caused the fault, and see what it must do to satisfy the
 * translation. Whether read from disk, or allocate a frame.
 **/
#define PAGING_LEAF_SWAPPED		(1<<13)
// This will help speed up detection of page that are guard mapped.
#define PAGING_LEAF_GUARDPAGE		(1<<14)
#define PAGING_LEAF_FAKEMAPPED		(1<<15)
/* When a dynamic allocation is partially fakemapped, the kernel can just
 * quickly map a page to the touched page.
 *
 * In the case of a shared library, or a partially mapped executable section,
 * the kernel has to know that data must be read from disk and filled in.
 * It would be much faster if the kernel could tell what kind of data must be
 * mapped in from the moment it gets the page fault.
 *
 * On #PF, if the page was fakemapped, the kernel will check for this flag.
 * If it's not set, the kernel will assume that the page can just take any old
 * frame, and will quickly allocate and map a new frame in and then return.
 *
 * If this flag is set, it would be a quick indicator to the kernel that the
 * fakemapped page pertains to a static code/data unmapped range. The kernel
 * must then consult the shared memory mappings and the executable section
 * data, and the memory mapped files descriptors, and see which one the page
 * needs to be filled in with.
 **/
#define PAGING_LEAF_STATICDATA		(1<<16)

#define PAGING_LEAF_SDATA_SHIFT		(17)
#define PAGING_LEAF_SDATA_MASK		(0x3)

#define PAGING_LEAF_EXEC_SECTION	(0x0)
#define PAGING_LEAF_MMAPPED_FILE	(0x1)
#define PAGING_LEAF_SHLIB		(0x2)

#ifndef __ASM__

//pagingLevel0S: x86 Top level paging structure, aka the Page Directory.
struct pagingLevel0S
{
	paddr_t		entries[PAGING_L0_NENTRIES];
#ifdef CONFIG_ARCH_x86_32_PAE
//Take note of the significance of this.
} __attribute__(( packed, aligned(32) ));
#else
} __attribute__(( packed, aligned(PAGING_BASE_SIZE) ));
#endif

struct pagingLevel1S
{
	paddr_t		entries[PAGING_L1_NENTRIES];
} __attribute__(( packed, aligned(PAGING_BASE_SIZE) ));

#ifdef CONFIG_ARCH_x86_32_PAE
struct pagingLevel2S
{
	paddr_t		entries[PAGING_L2_NENTRIES];
} __attribute__(( packed, aligned(PAGING_BASE_SIZE) ));
#endif

#endif /* !defined( __ASM__ ) */

#endif

