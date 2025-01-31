#ifndef _VADDR_SPACE_STREAM_H
	#define _VADDR_SPACE_STREAM_H

	#include <arch/paging.h>
	#include <__kclasses/stackCache.h>
	#include <kernel/common/stream.h>
	#include <kernel/common/vaddrSpace.h>
	#include <kernel/common/vSwamp.h>

/**	EXPLANATION:
 * The vaddrSpaceStream is and abstraction of the idea of Virtual Address Space
 * Management. It 'monitors' the virtual address space therefore, of its
 * associated process.
 *
 * Each process's vaddrSpaceStream provides virtual address caching,
 * and address space managment. The Virtual Address Space management is
 * provided by means of a helper class. The helper class is actually an
 * allocator for virtual addresses: it keeps a list of free virtual
 * addresses to be given out to the hosting memory stream when it requests
 * free virtual addresses from the process's address space.
 *
 * However, the swamp does not commit changes to the address space to the
 * hardware page tables. It just attempts to keep a coherent list of which
 * pages are _not_ occupied, and it is the kernel's responsibility to
 * ensure that those changes are reflected in the process's address space.
 *
 * See arch/walkerPageRanger.h, which is the kernel's page table walker,
 * responsible for committing changes to a virtual address space.
 **/

class ContainerProcess;

class VaddrSpaceStream
:
public Stream<ContainerProcess>
{
public:
	// This constructor must be used to initialize userspace streams.
	VaddrSpaceStream(
		uarch_t id, ContainerProcess *parent,
		void *swampStart, uarch_t swampSize)
	: Stream<ContainerProcess>(parent, id),
	vSwamp(swampStart, swampSize)
	{}

	// Provides the swamp with the right memory info to initialize.
	error_t initialize(void);

public:
	void *getPages(uarch_t nPages);
	void releasePages(void *vaddr, uarch_t nPages);

public:
	virtual error_t bind(void);
	virtual void cut(void);
	void dump(void);

public:
	VaddrSpace		vaddrSpace;
	StackCache<void *>	pageCache;
	VSwamp			vSwamp;
};

#endif

