
#include <__kstdlib/__kclib/string.h>
#include <kernel/common/task.h>
#include <kernel/common/cpuTrib/cpuStream.h>

// We make a global cpuStream for the bspCpu.
cpuStreamC		bspCpu(0, 0);

cpuStreamC::cpuStreamC(numaBankId_t bid, cpu_t cid)
:
cpuId(cid), bankId(bid)
{
	/**	EXPLANATION:
	 * There are three classes that have to check for initialization before
	 * constructing themselves:
	 *	1. cpuStreamC (this class).
	 *	2. processStreamC.
	 *	3. taskS.
	 *
	 * Of these three, two of them do not have constructor since they are
	 * structs and not classes. Why should these three classes have guard
	 * condition checks in their constructors?
	 *
	 * For the cpuStreamC class, there is a single global object
	 * (instantiated above). It is the statically allocated space for the
	 * bootstrap processor's most basic task and scheduling info.
	 *
	 * In this bootstrap CPU Stream is a pointer to the first and only
	 * thread that will be run during kernel bootstrap: thread ID 0x1,
	 * called the "Orientation" thread.
	 *
	 * This thread descriptor structure has an integer member, 'nLocksHeld'
	 * which is accessed every time the thread acquires or releases a lock.
	 * That means that the bootstrap CPU Stream must hold a valid pointer
	 * to the Orientation thread, or else any lock acquire will dereference
	 * the pointer to the current thread, and be accessing junk memory.
	 *
	 * Therefore there is an explicit initialization routine for the BSP
	 * CPU Stream, __korientationPreConstruct::bspInit() which will fill
	 * out the BSP statically allocated instance with a valid pointer to
	 * the orientation thread descriptor so that accesses will be valid.
	 *
	 * When the kernel's constuctors are called, this bootstrap CPU Stream,
	 * being (as necessitated by external referencing) a global object, will
	 * be constructed. Therefore we *MUST* place a guard here to stop any
	 * already initialized CPU Stream objects from being constructed.
	 *
	 * So to cut all the chat short, this check is vital to ensure that the
	 * BSP CPU Stream object is not overwritten.
	 **/
	if (initMagic == CPUSTREAM_INIT_MAGIC) {
		return;
	};
	initMagic = CPUSTREAM_INIT_MAGIC;

	memset(this, 0, sizeof(*this));
}

error_t cpuStreamC::initialize(void)
{
	return ERROR_SUCCESS;
}

cpuStreamC::~cpuStreamC(void)
{
}

