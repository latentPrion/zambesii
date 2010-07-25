#include <arch/arch_include.h>
#include ARCH_INCLUDE(paging.h)

#ifndef _ARCH_PAGING_COMMON
	#define _ARCH_PAGING_COMMON

#define PAGING_BYTES_TO_PAGES(__bytes)					\
	(((__bytes) / PAGING_BASE_SIZE) + (((__bytes) % PAGING_BASE_SIZE)?1:0))

#define PAGING_PAGES_TO_BYTES(__pages)	(PAGING_BASE_SIZE * (__pages))

#define PAGING_BASE_ALIGN_TRUNCATED(__addr)				\
	(((__addr) & PAGING_BASE_MASK_LOW) ? ((__addr) & PAGING_BASE_MASK_HIGH)\
		: (__addr))

#define PAGING_BASE_ALIGN_FORWARD(__addr)				\
	(((__addr) & PAGING_BASE_MASK_LOW) ?				\
		((((__addr)+PAGING_BASE_SIZE)) & PAGING_BASE_MASK_HIGH) :\
		(__addr))
		

#endif

