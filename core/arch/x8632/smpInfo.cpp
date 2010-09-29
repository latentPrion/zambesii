
#include <arch/smpInfo.h>
#include <commonlibs/acpi.h>
#include <commonlibs/x86mp.h>

chipsetNumaMapS *smpInfo::getNumaMap(void)
{
	/**	NOTES:
	 * This function will return a distinct chipsetNumaMapS *, which is
	 * in no way associated with the one returned from any chipset's
	 * getNumaMap().
	 *
	 * The kernel is expected to only parse for the cpu related entries
	 * when it asks the smpInfo:: namespace for a NUMA Map anyway.
	 *
	 *	EXPLANATION:
	 * This function works by first scanning for the ACPI RSDP. If it finds
	 * it, it will then proceed to parse until it finds an SRAT. If it finds
	 * one, it will then parse for any CPU entries and generate a NUMA
	 * map of all CPUs present in any particular bank.
	 *
	 * This function depends on the kernel libacpi.
	 **/
	return __KNULL;
}

archSmpMapS *smpInfo::getSmpMap(void)
{
	/**	NOTES:
	 *  This function will return a structure describing all CPUs at boot,
	 * regardless of NUMA bank membership, and regardless of whether or not
	 * that CPU is enabled, bad, or was already described in the NUMA Map.
	 *
	 * This SMP map is used to generate shared bank CPUs (CPUs which were
	 * not mentioned in the NUMA Map, yet are mentioned here), and to tell
	 * the kernel which CPUs are bad, and in an "un-enabled", or unusable
	 * state, though not bad.
	 *
	 *	EXPLANATION:
	 * This function will use the Intel x86 MP Specification structures to
	 * enumerate CPUs, and build the SMP map, combining it with the info
	 * returned from the NUMA map.
	 *
	 * This function depends on the kernel libx86mp.
	 **/
	return __KNULL;
}

