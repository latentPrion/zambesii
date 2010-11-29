
#include <__kclasses/debugPipe.h>
#include <kernel/common/machineAffinity.h>


error_t affinity::copyLocal(localAffinityS *dest, localAffinityS *src)
{
	error_t		ret;

	src->def.rsrc = dest->def.rsrc;

	ret = dest->memBanks.initialize(src->memBanks.getNBits());
	if (ret != ERROR_SUCCESS) {
		return ret;
	};
	dest->memBanks.merge(&src->memBanks);

	ret = dest->cpuBanks.initialize(src->cpuBanks.getNBits());
	if (ret != ERROR_SUCCESS) {
		return ret;
	};
	dest->cpuBanks.merge(&src->cpuBanks);

	ret = dest->cpus.initialize(src->cpus.getNBits());
	if (ret != ERROR_SUCCESS) {
		return ret;
	};
	dest->cpus.merge(&src->cpus);

	return ERROR_SUCCESS;
}

error_t affinity::copyMachine(affinityS *dest, affinityS *src)
{
	error_t		ret;

	dest->nEntries = 0;

	dest->machines = new localAffinityS[src->nEntries];
	if (dest->machines == __KNULL) {
		return ERROR_MEMORY_NOMEM;
	};

	for (uarch_t i=0; i<src->nEntries; i++)
	{
		ret = affinity::copyLocal(
			&dest->machines[i], &src->machines[i]);

		if (ret != ERROR_SUCCESS) {
			return ret;
		};
		dest->nEntries++;
	};

	return ERROR_SUCCESS;
}

