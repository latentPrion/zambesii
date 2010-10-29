#ifndef _UDI_CB_H
	#define _UDI_CB_H

	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/slamCache.h>

/**	EXPLANATION:
 * To the kernel, a CB group is nothing more than a cache. The kernel will
 * take any control block group, and determine its size, and then create a new
 * object cache for it.
 *
 * The UDI specification does not guarantee that control block index numbers
 * will be sequential (this would have been a very good idea) so we effectively
 * can't use arrays and have to use linked lists, which are slower. Great.
 *
 * TODO: If you can, try to have them consider making all of the "idx" style
 * fields be required to be sequential starting from 0 or 1. Being able to just
 * index into arrays is much faster.
 **/

struct udiCbDescS
{
	ubit16		index;
	ubit32		size;
	slamCacheC	*cache;
};

#endif

