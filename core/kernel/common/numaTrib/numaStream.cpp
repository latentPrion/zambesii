
#include <__kclasses/debugPipe.h>
#include <kernel/common/numaTrib/numaStream.h>

/* For these bind()/cut() operations, we don't monitor the binding status. Let\
 * the individual bank classes monitor themselves.
 *
 * TODO:
 * When cutting a NUMA bank, it's important to update the global default config
 * and check to see if the default bank is the one being taken down. If so, the
 * default bank must be changed. So a callback must be registered somewhere
 * along the execution path of numaStreamC::cut() to ensure that stale banks
 * don't remain the default banks for all kernel allocations.
 **/

numaStreamC::numaStreamC(void)
{
}

numaStreamC::numaStreamC(
	numaBankId_t bankId, paddr_t baseAddr, paddr_t size,
	void *preAllocated
	)
{
	initialize(bankId, baseAddr, size, preAllocated);
}

error_t numaStreamC::initialize(
	numaBankId_t bankId, paddr_t baseAddr, paddr_t size,
	void *preAllocated)
{
	error_t		ret;

	streamC::id = 0;
	this->bankId = bankId;

	ret = memoryBank.initialize(baseAddr, size, preAllocated);
	if (ret != ERROR_SUCCESS) {
		return ret;
	};

	return ERROR_SUCCESS;
}

void numaStreamC::cut(void)
{
	// memoryBank.cut();
	// cpuBank.cut();
}

void numaStreamC::bind(void)
{
	// memoryBank.bind();
	// cpuBank.bind();
}


