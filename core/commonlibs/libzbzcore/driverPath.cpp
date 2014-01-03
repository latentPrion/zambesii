
#include <__kstdlib/__kcxxlib/memory>
#include <kernel/common/process.h>
#include <kernel/common/taskTrib/taskTrib.h>
#include <kernel/common/floodplainn/floodplainn.h>
#include <commonlibs/libzbzcore/libzbzcore.h>


void driverPath0(threadC *self);
void __klibzbzcoreDriverPath(threadC *self)
{
//	fplainn::driverC				*driver;
	streamMemPtrC<messageStreamC::iteratorS>	msgIt;
	syscallbackC					*callback;

	/**	EXPLANATION:
	 * This is essentially like a kind of udi_core lib.
	 *
	 * We will check all requirements and ensure they exist; then we'll
	 * proceed to spawn regions, etc. We need to do a message loop because
	 * we will have to do async calls to the ZUDI Index server, and possibly
	 * other subsystems.
	 **/

	msgIt = new (self->parent->memoryStream.memAlloc(
		PAGING_BYTES_TO_PAGES(sizeof(*msgIt))))
		messageStreamC::iteratorS;

	if (msgIt.get() == NULL)
	{
		printf(FATAL LZBZCORE"driverPath: proc 0x%x: Failed to alloc "
			"mem for msg iterator.\n",
			self->getFullId());

		self->parent->sendResponse(ERROR_MEMORY_NOMEM);
		taskTrib.dormant(self->getFullId());
	};

	driverPath0(self);

	for (;;)
	{
		self->getTaskContext()->messageStream.pull(msgIt.get());
		callback = (syscallbackC *)msgIt->header.privateData;

		switch (msgIt->header.subsystem)
		{
		default:
			if (callback == NULL)
			{
				printf(WARNING LZBZCORE"driverPath: message "
					"with no callback continuation.\n");

				break;
			};

			(*callback)(msgIt.get());
			asyncContextCache->free(callback);
			break;
		};
	};
}

static syscallbackDataF driverPath1;
void driverPath0(threadC *self)
{
	zudiIndexServer::loadDriverRequirementsReq(
		self->parent->fullName, newSyscallback(&driverPath1));

}

void driverPath1(messageStreamC::iteratorS *msgIt, void *)
{
	threadC			*self;
	const driverInitEntryS	*driverInit;
	fplainn::deviceC	*dev;

	self = (threadC *)cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentTask();

	// Not error checking this.
	floodplainn.getDevice(self->parent->fullName, &dev);
	printf(NOTICE LZBZCORE"driverPath1: Ret from loadRequirementsReq: %s "
		"(%d).\n",
		strerror(msgIt->header.error), msgIt->header.error);

	/**	EXPLANATION:
	 * In the userspace libzbzcore, we would have to make calls to the VFS
	 * to load and link all of the metalanguage libraries, as well as the
	 * driver module itself.
	 *
	 * However, since this is the kernel syslib, we can skip all of that,
	 * and move directly into getting the udi_init_t structures for all of
	 * the components involved.
	 **/
	driverInit = floodplainn.findDriverInitInfo(dev->driver->shortName);
	if (driverInit == NULL)
	{
		printf(NOTICE LZBZCORE"Failed to find udi_init_info for %s.\n",
			dev->driver->shortName);

		return;
	};

	
	self->parent->sendResponse(msgIt->header.error);
}

