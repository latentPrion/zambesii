
#include <arch/arch.h>
#include <arch/paging.h>
#include <chipset/memory.h>
#include <__kstdlib/__kmath.h>

// Reserve space for the __kspaceBmp within the kernel image:
uarch_t		__kspaceInitMem[
	__KMATH_NELEMENTS(
		(CHIPSET_MEMORY___KSPACE_SIZE / PAGING_BASE_SIZE),
		(sizeof(uarch_t) * __BITS_PER_BYTE__))];

hardwareIdListC<numaStreamC>::arrayNodeS	__kspaceStreamPtr[
	CHIPSET_MEMORY_NUMA___KSPACE_BANKID + 1];

