#ifndef _PAGE_TABLE_CACHE_H
	#define _PAGE_TABLE_CACHE_H

	#include <arch/paddr_t.h>
	#include <arch/paging.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/allocClass.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>

/**	EXPLANATION:
 * Page Table Cache is a fast one-frame allocator for on-the-fly allocation of
 * paging structures. Right now it can only hold as many frame addresses as can
 * be contained in one page. Soon, it will be a runtime-expanding class which
 * is dynamically grown and shrunk to cater for the kernel's page-structure
 * needs.
 **/

#define PTCACHE_NENTRIES	(PAGING_BASE_SIZE / sizeof(paddr_t))
#define PTCACHE_STACK_EMPTY	(-1)
#define PTCACHE_STACK_FULL	(PTCACHE_NENTRIES - 1)

class pageTableCacheC
:
public allocClassC
{
public:
	pageTableCacheC(void);
	~pageTableCacheC(void);

public:
	error_t pop(paddr_t *paddr);
	void push(paddr_t paddr);

	// This will flush the cache when pmem is lacking. Pretty expensive.
	void flush(void);

private:
	paddr_t		stack[PTCACHE_NENTRIES];
	sharedResourceGroupC<waitLockC, sarch_t>	stackPtr;
};

#endif

