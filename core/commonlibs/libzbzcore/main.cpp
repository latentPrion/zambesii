
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

void __klzbzcore::main()
{
	Thread		*self;
	error_t		err;

	self = (Thread *)cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentTask();

	printf(NOTICE LZBZCORE"New process entered. Pid 0x%x, type %d.\n",
		self->getFullId(), self->parent->getType());

	switch (self->parent->getType())
	{
	case ProcessStream::DISTRIBUTARY:
		err = __klzbzcore::distributary::main(self);
		break;

	case ProcessStream::DRIVER:
		err = __klzbzcore::driver::main(self);
		break;

	default:
		err = ERROR_UNKNOWN;
		printf(NOTICE LZBZCORE"Proc 0x%x: Process type not supported. "
			"Dormanting.\n",
			self->getFullId());

		break;
	};

	self->parent->sendResponse(err);
	printf(NOTICE LZBZCORE"Proc 0x%x: Done executing. Dormanting.\n",
		self->getFullId());

	taskTrib.dormant(self->getFullId());
}

