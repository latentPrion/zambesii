#ifndef _WALKER_PAGE_RANGER_ACCESSORS_H
	#define _WALKER_PAGE_RANGER_ACCESSORS_H

	#include <arch/x8632/paddr_t.h>
	#include <arch/x8632/paging.h>

/**	EXPLANATION:
 * These global variables are used to hold a constant set of values across all
 * CPUs which will access the accessor or modifier page for the relevant level
 * of the paging hierarchy.
 **/

// Accessors.
extern volatile sPagingLevel1	*const level1Accessor;
#ifdef CONFIG_ARCH_x86_32_PAE
extern volatile sPagingLevel2	*const level2Accessor;
#endif

// Modifiers.
extern paddr_t		*const level1Modifier;
#ifdef CONFIG_ARCH_x86_32_PAE
extern paddr_t		*const level2Modifier;
#endif

#endif

