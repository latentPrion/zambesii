
#include <lang/lang.h>

utf8Char		*mmStr[] =
{
	// [0]
	"No memory returned from __kspaceMemAlloc() when a memBmpC instance \
		called on it while constructing. Allocation of the internal \
		bitmap failed inside of a memBmp constructor.",
	// [1]:
	"In a call to memoryTribC::rawMemAlloc(), the kernel when mapping in \
		a set of frames, walkerPageRanger::__kdataMap() returned less \
		than expected for nMapped.",
	// [2]:
	"In a call to memoryTribC::rawMemAlloc(), the kernel, when mapping in \
		a set of frames, walkerPageRanger::__kfakeMap() returned less \
		than expected for nMapped.",
	// [3]:
	"In a call to memoryTribC::__kspaceMemAlloc(), the kernel, when \
		mapping in a set of frames, got an unexpected number of \
		mapped frames from walkerPageRanger.",
	// [4]:
	"In a call to memoryStreamC::real_memAlloc(), the kernel, when calling \
		walkerPageRanger::mapInc(), got a status which indicated that \
		not all frames were mapped."
	// [5]:
	"In a call to memoryStreamC::real_memAlloc(), the kernel, when calling \
		walkerPageRanger::mapNoInc() (fakemapping), got a status which \
		indicated that not all frames were mapped."
};

