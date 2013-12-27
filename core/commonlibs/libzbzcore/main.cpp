
#include <__kstdlib/__ktypes.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/taskTrib/taskTrib.h>
#include <commonlibs/libzbzcore/libzbzcore.h>


/**	EXPLANATION:
 * See "notes.txt" within this folder for a detailed explanation.
 *
 * 1. getProcessInitializationBlock(): Get process type.
 * 2. If:
 *	a. Driver process: Enter lib __kudi.
 *	b. Distributary process: Enter the distributary.
 **/

void __klibzbzcoreEntry()
{
	threadC		*self;

	self = (threadC *)cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentTask();

	printf(NOTICE LZBZCORE"New process entered. Pid 0x%x, type %d.\n",
		self->getFullId(), self->parent->getType());

	switch (self->parent->getType())
	{
	case processStreamC::DISTRIBUTARY:
		__klibzbzcoreDistributaryPath(self);
		break;

	case processStreamC::DRIVER:
		__klibzbzcoreDriverPath(self);
		break;

	default:
		printf(NOTICE LZBZCORE"Proc 0x%x: Process type not supported. "
			"Dormanting.\n",
			self->getFullId());

		taskTrib.dormant(self->getFullId());
		break;
	};

	printf(NOTICE LZBZCORE"Proc 0x%x: Fell out of path. "
		"Dormanting.\n",
		self->getFullId());

	taskTrib.dormant(self->getFullId());
}

