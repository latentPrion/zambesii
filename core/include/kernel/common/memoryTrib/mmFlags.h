#ifndef _MEMORY_MANAGER_FLAGS_H
	#define _MEMORY_MANAGER_FLAGS_H

/**	EXPLANATION:
 * These flags define the behaviour of the Memory Stream class, and the NUMA
 * Memory Banks when allocating memory for a process.
 *
 * The NUMA Tributary will have to be expanded to allow a more in-depth
 * allocation sequence which takes into account all of these flags.
 *
 * The Memory Stream must be able to take all of its relevant options as well:
 *	* If the caller wants the whole allocation to be fakemapped, and not
 *	  backed by physical memory.
 *	* Caller wants to ensure that none of the range of memory is fakemapped,
 *	  and all of the virtual memory returned is completely backed by
 *	  physical memory.
 *
 * Most of these flags are to do with physical memory allocation though.
 *	* Caller requests that the allocation not be put to sleep if physical
 *	  memory is unavailable. This is useful for allocations from IRQ
 *	  context. However, simply because you don't sleep and you wait doesn't
 *	  mean that you're going to get memory faster.
 *	* Caller specifically asks that the request not go to swap for physical
 *	  memory if all fails.
 **/

#define MEMALLOC_NO_FAKEMAP		(1<<0)
// Can be thought of as a request to return fail status on failure.
#define MEMALLOC_NO_SLEEP		(1<<1)
#define MEMALLOC_NO_TRY_SWAP		(1<<2)
#define MEMALLOC_PURE_VIRTUAL		(1<<3)


#endif

