
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
static void driverMain1(Thread *self, error_t ret);
static void distributaryMain1(Thread *self, error_t ret);

void __klzbzcore::main()
{
	Thread		*self;

	self = (Thread *)cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentThread();

	printf(NOTICE LZBZCORE"New process entered. Pid %x, type %d.\n",
		self->getFullId(), self->parent->getType());

	switch (self->parent->getType())
	{
	case ProcessStream::DISTRIBUTARY:
		__klzbzcore::distributary::main(self, &::distributaryMain1);
		break;

	case ProcessStream::DRIVER:
		__klzbzcore::driver::main(self, &::driverMain1);
		break;

	default:
		printf(NOTICE LZBZCORE"Proc %x: Process type not supported. "
			"Dormanting.\n",
			self->getFullId());

		self->parent->sendResponse(ERROR_INVALID_FORMAT);
		break;
	};

	/* Can only reach here if the message loop in driver::main or
	 * distributary::main exits.
	 **/
	printf(FATAL LZBZCORE"Proc %x: Fell through to end of "
		"__klzbzcore:main. Killing.\n",
		self->getFullId());

	taskTrib.kill(self->getFullId());
}

static void driverMain1(Thread *self, error_t ret)
{
	self->parent->sendResponse(ret);
	printf(NOTICE LZBZCORE"Driver Proc %x: Done executing. Killing with "
		"ret %s.\n",
		self->getFullId(), strerror(ret));

	if (ret != ERROR_SUCCESS) {
		taskTrib.kill(self->getFullId());
	};
}

static void distributaryMain1(Thread *self, error_t ret)
{
	self->parent->sendResponse(ret);
	printf(NOTICE LZBZCORE"Dtrib Proc %x: Done executing. Killing with "
		"ret %s.\n",
		self->getFullId(), strerror(ret));

	if (ret != ERROR_SUCCESS) {
		taskTrib.kill(self->getFullId());
	};
}
