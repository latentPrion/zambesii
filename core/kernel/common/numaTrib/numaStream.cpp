
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

numaStreamC::numaStreamC(numaBankId_t bankId)
:
streamC(0), bankId(bankId)
{
}

numaStreamC::~numaStreamC(void)
{
}

error_t numaStreamC::initialize(uarch_t nCpuBits)
{
	return cpus.initialize(nCpuBits);
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

void numaStreamC::dump(void)
{
	// Print each bit.
	__kprintf(NOTICE NUMASTREAM"CPUs:\n\t");
	for (uarch_t i=0; i<cpus.getNBits(); i++)
	{
		if (cpus.testSingle(i)) {
			__kprintf((utf8Char *)"%d ", i);
		};
	};

	for (uarch_t i=0; i<memBanks.getNBits(); i++)
	{
		if (memBanks.testSingle(i)) {
			__kprintf((utf8Char *)"%d ", i);
		};
	};
}

