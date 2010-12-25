
#include <__kstdlib/__kclib/string8.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/machineAffinity.h>


error_t affinity::copyLocal(localAffinityS *dest, localAffinityS *src)
{
	error_t		ret;

	dest->name = new utf8Char[strlen8(src->name) + 1];
	if (dest->name == __KNULL && src->name != __KNULL) {
		return ERROR_MEMORY_NOMEM;
	};
	strcpy8(dest->name, src->name);

	dest->def.rsrc = src->def.rsrc;

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

localAffinityS *affinity::findLocal(affinityS *ma, utf8Char *localName)
{
	for (ubit32 i=0; i<ma->nEntries; i++)
	{
		if (strcmp8(ma->machines[i].name, localName) == 0) {
			return &ma->machines[i];
		};
	};
	return __KNULL;
}

