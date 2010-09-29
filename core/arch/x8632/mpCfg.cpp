
#include <arch/x8632/mp.h>
#include <arch/walkerPageRanger.h>
#include <__kstdlib/__kcxxlib/new>
#include <kernel/common/memoryTrib/memoryTrib.h>


/* I want the MP code to produce, in the end, a CPU map and a CPU Config
 * structure.
 *
 * The MP code must find an MP floating pointer and see if there is an MP
 * config table. The MP code must, if there is a config table, parse it and
 * generate the CPU config and map structures.
 */

x86_mpFpS *x86Mp::findMpFp(void)
{
	/* This function must call on the chipset for help. The chipset must
	 * scan for the MP FP structure in its own chipset specific manner.
	 *
	 * This function should not fail if there is MP on the board.
	 **/
	return chipset_x86FindMpFp();
}

x86_mpCfgS *x86Mp::getMpCfgTable(x86_mpFpS *mpfp)
{
	ubit32		cfgPaddr = mpfp->cfgTablePaddr;
	x86_mpCfgS	*ret;

	// Map the cfg table into the kernel vaddrspace.
	ret = new ((memoryTrib.__kmemoryStream.vaddrSpaceStream
		.*memoryTrib.__kmemoryStream.vaddrSpaceStream.getPages)(1))
		x86MpCfgS;

	walkerPageRanger::mapInc(
		ret, cfgPaddr, 

status_t x86Mp::getChipsetDefaultConfig(x86_mpFpS *mpfp)
{
	/* Determines whether or not the chipset uses an MP default config.
	 * Returns negative value if not, returns value greater than 0 if yes.
	 *
	 * The value returned, if greater than 0, is an index into a hardcoded
	 * table of CPU Config and CPU Maps to return for each default config.
	 **/
	if (mpfp->features[0] == 0) {
		return -1;
	};
	return mpfp->features[0];
}



ubit32 x86Mp::getLapicPaddr(x86_mpCfgS *cfg)
{
	// Returns the physical address of the LAPIC per CPU.
	return cfg->lapicPaddr;
}

ubit16 x86Mp::getNConfigEntries(x86_mpCfgS *cfg)
{
	return cfg->nEntries;
}

uarch_t x86Mp::getChipsetNCpus(x86_mpCfgS *cfg)
{
	
