
#include <__kstdlib/__kcxxlib/memory>
#include <kernel/common/process.h>
#include <kernel/common/taskTrib/taskTrib.h>
#include <kernel/common/floodplainn/floodplainn.h>
#include <commonlibs/libzbzcore/libzbzcore.h>


void __klibzbzcoreDriverPath(threadC *self)
{
//	fplainn::driverC				*driver;
	streamMemPtrC<messageStreamC::iteratorS>	msgIt;

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

	// This should not be here in the final implementation.
	self->parent->sendResponse(ERROR_SUCCESS);

	for (;;)
	{
		self->getTaskContext()->messageStream.pull(msgIt.get());
		printf(NOTICE LZBZCORE"driverPath: proc 0x%x: got message.\n",
			self->getFullId());
	};
}
