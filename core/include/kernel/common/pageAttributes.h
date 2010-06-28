#ifndef _PAGE_ATTRIBUTES_H
	#define _PAGE_ATTRIBUTES_H

#define PAGEATTRIB_EXECUTABLE			(1<<3)
#define PAGEATTRIB_WRITE			(1<<1)
#define PAGEATTRIB_SUPERVISOR			(1<<2)
#define PAGEATTRIB_PRESENT			(1<<4)
#define PAGEATTRIB_ACCESSED			(1<<5)
#define PAGEATTRIB_DIRTY			(1<<6)
#define PAGEATTRIB_TLB_GLOBAL			(1<<7)
#define PAGEATTRIB_CACHE_WRITE_THROUGH		(1<<9)
#define PAGEATTRIB_CACHE_DISABLED		(1<<8)

/* This flag is essential to ensuring that mappings use __kspace before the
 * NUMA Tributary is initialized.
 **/
#define WPRANGER_FLAGS___KSPACE			(1<<10)

#endif

