
#include <__ksymbols.h>
#include <chipset/zkcm/zkcmCore.h>
#include <__kstdlib/__ktypes.h>
#include <__kstdlib/compiler/cxxrtl.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/string.h>
#include <__kclasses/debugPipe.h>
#include <__kclasses/memReservoir.h>
#include <__kthreads/__korientation.h>
#include <kernel/common/__koptimizationHacks.h>
#include <kernel/common/timerTrib/timerTrib.h>
#include <kernel/common/interruptTrib/interruptTrib.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/taskTrib/taskTrib.h>
#include <kernel/common/vfsTrib/vfsTrib.h>
#include <kernel/common/distributaryTrib/distributaryTrib.h>

#include <arch/cpuControl.h>


int oo=0, pp=0, qq=0, rr=0;

static utf8Char		fullName[PROCESS_FULLNAME_MAXLEN],
			arguments[PROCESS_ARGUMENTS_MAXLEN],
			ename[PROCESS_ENV_NAME_MAXLEN+1],
			eval[PROCESS_ENV_VALUE_MAXLEN+1];

extern "C" void __korientationInit(ubit32, multibootDataS *)
{
	error_t		ret;
	uarch_t		devMask;
	processId_t	tid;
	containerProcessC	&__kprocess = *processTrib.__kgetStream();

	/* Zero out uninitialized sections, prepare kernel locking and place a
	 * pointer to the BSP CPU Stream into the BSP CPU; then we can call all
	 * C++ global constructors.
	 **/
	__koptimizationHacks();
	memset(&__kbssStart, 0, &__kbssEnd - &__kbssStart);
	DO_OR_DIE(bspCpu, initializeBspCpuLocking(), ret);
	cxxrtl::callGlobalConstructors();

	/* Initialize exception handling, then do chipset-wide early init.
	 * Finally, initialize the irqControl mod, and mask all IRQs off to
	 * place the chipset into a stable state.
	 **/
	DO_OR_DIE(interruptTrib, initializeExceptions(), ret);
	DO_OR_DIE(zkcmCore, initialize(), ret);
	DO_OR_DIE(zkcmCore.irqControl, initialize(), ret);
	zkcmCore.irqControl.maskAll();
	DO_OR_DIE(processTrib, initialize(), ret);
	DO_OR_DIE(
		__kprocess,
		initialize(CC"@h:boot/zambesii/zambesii.zxe",
		CC"0\0001\0PATH=@h:__kramdisk/programs32\0DRIVERPATH=@h:__kramdisk/zambesii/drivers32\0foo\0bar=\0", __KNULL), ret);

	/* Initialize __kspace level physical memory management, then the
	 * kernel Memory Stream.
	 **/
	DO_OR_DIE(memoryTrib, initialize(), ret);
	DO_OR_DIE(memoryTrib, __kspaceInitialize(), ret);
	DO_OR_DIE(processTrib.__kgetStream()->memoryStream, initialize(), ret);

	/* Initialize the kernel debug pipe for boot logging, etc.
	 **/
	DO_OR_DIE(__kdebug, initialize(), ret);
	devMask = __kdebug.tieTo(DEBUGPIPE_DEVICE_BUFFER | DEBUGPIPE_DEVICE1);
	if (!__KFLAG_TEST(devMask, DEBUGPIPE_DEVICE_BUFFER)) {
		__kprintf(WARNING ORIENT"No debug buffer allocated.\n");
	};

	__kdebug.refresh();
	__kprintf(NOTICE ORIENT"Kernel debug output tied to devices BUFFER and "
		"DEVICE1.\n");

	for (ubit8 i=0; i<__kprocess.nEnvVars; i++)
	{
		strncpy8(ename, __kprocess.environment[i].name, PROCESS_ENV_NAME_MAXLEN);
		ename[PROCESS_ENV_NAME_MAXLEN] = '\0';
		strncpy8(eval, __kprocess.environment[i].value, PROCESS_ENV_VALUE_MAXLEN);
		eval[PROCESS_ENV_VALUE_MAXLEN] = '\0';

		__kprintf(NOTICE"Env %d of %d: Name %s.\n\tVal %s.\n", i, __kprocess.nEnvVars, ename, eval);
	};
for (;;){};

	/* Initialize the kernel Memory Reservoir (heap) and object cache pool.
	 **/
	DO_OR_DIE(memReservoir, initialize(), ret);
	DO_OR_DIE(cachePool, initialize(), ret);

	/* Initialize the CPU Tributary's internal BMPs etc, then initialize the
	 * BSP CPU's scheduler to co-op level scheduling capability, and
	 * spawn the __korientationMain thread, ending __korientationInit.
	 **/
	DO_OR_DIE(cpuTrib, initialize(), ret);
	DO_OR_DIE(zkcmCore.cpuDetection, initialize(), ret);
	DO_OR_DIE(cpuTrib, initializeBspCpuStream(), ret);

	/* Spawn the new thread for __korientationMain. There is no need to
	 * unschedule __korientationInit() because it will never be scheduled.
	**/
	DO_OR_DIE(
		__kprocess,
		spawnThread(
			(void (*)(void *))&__korientationMain, __KNULL,
			__KNULL,
			taskC::ROUND_ROBIN,
			0,
			SPAWNTHREAD_FLAGS_AFFINITY_PINHERIT,
			&tid),
		ret);

	cpuTrib.getCurrentCpuStream()->taskStream.pull();
}

void __korientationMain(void)
{
	error_t			ret;

	/* Initialize Interrupt Trib IRQ management (__kpin and __kvector),
	 * then load the chipset's bus-pin mappings and initialize timer
	 * services.
	 **/
	DO_OR_DIE(interruptTrib, initializeIrqManagement(), ret);
	DO_OR_DIE(zkcmCore.irqControl.bpm, loadBusPinMappings(CC"isa"), ret);
	DO_OR_DIE(zkcmCore.timerControl, initialize(), ret);
	DO_OR_DIE(timerTrib, initialize(), ret);

	// Detect physical memory.
	DO_OR_DIE(zkcmCore.memoryDetection, initialize(), ret);
	DO_OR_DIE(memoryTrib, pmemInit(), ret);

	// Detect and wake all CPUs.
	DO_OR_DIE(cpuTrib, initializeAllCpus(), ret);

	/* Initialize the VFS Trib to enable us to begin constructing the
	 * various currentts, and then populate the distributary namespace.
	 **/
	DO_OR_DIE(vfsTrib, initialize(), ret);
	DO_OR_DIE(distributaryTrib, initialize(), ret);


	__kprintf(NOTICE ORIENT"Successful; about to dormant.\n");
	__kdebug.refresh();
	taskTrib.dormant(
		cpuTrib.getCurrentCpuStream()->taskStream.getCurrentTask());
}

