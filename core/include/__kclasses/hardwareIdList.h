#ifndef _HARDWARE_ID_LIST_H
	#define _HARDWARE_ID_LIST_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/processId.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/multipleReaderLock.h>

#define HWIDLIST_INDEX_INVALID		(-1)

#define HWIDLIST_FLAGS_INDEX_VALID	(1<<0)

class hardwareIdListC
{
public:
	hardwareIdListC(void);
	void __kspaceSetState(sarch_t id, void *arrayMem);
public:
	// Retrieves an item's pointer by its hardware ID.
	void *getItem(sarch_t id);

	error_t addItem(sarch_t id, void *item);
	void removeItem(sarch_t id);

	// Custom just for the process list.
	error_t findFreeProcess(processId_t *id);
	uarch_t	getNProcesses(void)
	{
		return static_cast<uarch_t>( arr.rsrc.maxIndex + 1 );
	};

	/* Allows a caller to loop through the array without knowing about
	 * the layout of the members.
	 **/
	sarch_t prepareForLoop(void);
	void *getLoopItem(sarch_t *id);

public:
	struct arrayNodeS
	{
		sarch_t		next;
		uarch_t		flags;
		void		*item;
	};

private:
	struct arrayStateS
	{
		arrayNodeS	*arr;
		sarch_t		maxIndex, firstValidIndex;
	};
	sharedResourceGroupC<multipleReaderLockC, arrayStateS>	arr;
};

#endif

