#ifndef _V_SWAMP_H
	#define _V_SWAMP_H

	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/swampInfoNode.h>
	#include <__kclasses/__kequalizerList.h>
	#include <kernel/common/recursiveLock.h>
	#include <kernel/common/sharedResourceGroup.h>

#define VSWAMP_NSWAMPS			2
#define VSWAMP_INSERT_BEFORE		-1
#define VSWAMP_INSERT_AFTER		-2

class vSwampC
{
public: class holeMapS;
private: struct swampS;

public:
	// Constructor for userspace swamps.
	vSwampC(void *swampStart, uarch_t swampSize, holeMapS *holeMap);
	// Constructor for the kernel's swamp.
	vSwampC(void);
	// This is the final step in initializing the kernel Vaddrspace.
	error_t initialize(
		void *swampStart, uarch_t swampSize, holeMapS *holeMap);

	~vSwampC(void);

public:
	void *getPages(uarch_t nPages);
	void releasePages(void *vaddr, uarch_t nPages);

	void dump(void);

private:
	swampInfoNodeC *getNewSwampNode(
		void *baseAddr, uarch_t size,
		swampInfoNodeC *prev, swampInfoNodeC *next);

	void deleteSwampNode(swampInfoNodeC *node);

	swampInfoNodeC *findInsertionNode(
		swampS *swamp, void *vaddr, status_t *status);

public:		
	struct holeMapS
	{
		uarch_t	baseAddr;
		uarch_t	size;
	};

private:
	struct swampStateS
	{
		/* The tail pointer will become necessary when we implement
		 * stack allocation, where we traverse the swamp from the tail
		 * so as to get pages as high up in the address space as
		 * possible.
		 **/
		swampInfoNodeC	*head, *tail;
	};
	struct swampS
	{
		uarch_t		baseAddr;
		uarch_t		size;
		uarch_t		flags;
		sharedResourceGroupC<recursiveLockC, swampStateS>	ptrs;
	};
	// Initial swamp info nodes.
	swampInfoNodeC		initSwampNodes[VSWAMP_NSWAMPS];
	// Each process has two swamps.
	swampS			swamps[VSWAMP_NSWAMPS];
	__kequalizerListC<swampInfoNodeC>		swampNodeList;
};

#endif

