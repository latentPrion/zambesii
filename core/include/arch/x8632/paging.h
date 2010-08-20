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

#define PAGESTATUS_SHIFT			(12)
#define PAGETYPE_SHIFT				(PAGESTATUS_SHIFT + 3)

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

