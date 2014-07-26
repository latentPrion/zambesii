#ifndef _PROCESS_TRIBUTARY_H
	#define _PROCESS_TRIBUTARY_H

	#include <chipset/memory.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/wrapAroundCounter.h>
	#include <kernel/common/process.h>
	#include <kernel/common/thread.h>
	#include <kernel/common/tributary.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/multipleReaderLock.h>
	#include <kernel/common/processId.h>

#define PROCTRIB			"Proc Trib: "

#define PROCESSTRIB_UPDATE_ADD		0x0
#define PROCESSTRIB_UPDATE_SUBTRACT	0x1
#define PROCESSTRIB_UPDATE_SET		0x2

/**	Flag values for ProcessTrib::spawnProcess().
 **/
// STINHERIT by default.
#define SPAWNPROC_FLAGS_AFFINITY_STINHERIT	SPAWNTHREAD_FLAGS_AFFINITY_STINHERIT
// Implied if the cpuAffinity BMP != NULL && !PINHERIT
#define SPAWNPROC_FLAGS_AFFINITY_SET		SPAWNTHREAD_FLAGS_AFFINITY_SET
#define SPAWNPROC_FLAGS_AFFINITY_PINHERIT	SPAWNTHREAD_FLAGS_AFFINITY_PINHERIT

// STINHERIT by default.
#define SPAWNPROC_FLAGS_SCHEDPOLICY_STINHERIT	SPAWNTHREAD_FLAGS_SCHEDPOLICY_STINHERIT
#define SPAWNPROC_FLAGS_SCHEDPOLICY_SET		SPAWNTHREAD_FLAGS_SCHEDPOLICY_SET

// Set to PRIOCLASS_DEFAULT by default.
#define SPAWNPROC_FLAGS_SCHEDPRIO_CLASS_DEFAULT	SPAWNTHREAD_FLAGS_SCHEDPRIO_CLASS_DEFAULT
#define SPAWNPROC_FLAGS_SCHEDPRIO_STINHERIT	SPAWNTHREAD_FLAGS_SCHEDPRIO_STINHERIT
#define SPAWNPROC_FLAGS_SCHEDPRIO_PRIOCLASS_SET	SPAWNTHREAD_FLAGS_SCHEDPRIO_PRIOCLASS_SET
#define SPAWNPROC_FLAGS_SCHEDPRIO_SET		SPAWNTHREAD_FLAGS_SCHEDPRIO_SET

#define SPAWNPROC_FLAGS_DORMANT			SPAWNTHREAD_FLAGS_DORMANT

/**	Return values for ProcessTrib::spawnProcess().
 **/
#define SPAWNPROC_STATUS_INVALID_FILE_NAME	(0x1)
#define SPAWNDRIVER_STATUS_NO_DRIVER		(0x2)
#define SPAWNDRIVER_STATUS_METAS_MISSING	(0x3)

class ProcessTrib
:
public Tributary
{
public:
	ProcessTrib(void *vaddrSpaceBaseAddr, uarch_t vaddrSpaceSize)
	:
	__kprocess(vaddrSpaceBaseAddr, vaddrSpaceSize),
	// Kernel occupies process ID 1; we begin handing process IDs from 2.
	nextProcId(
		CHIPSET_MEMORY_MAX_NPROCESSES - 1,
		PROCID_PROCESS(__KPROCESSID) + 1)
	{
		memset(processes.rsrc, 0, sizeof(processes.rsrc));
		/**	EXPLANATION:
		 * I am not a fan of mnemonic magic numbers, but in this case
		 * I will use this here. The kernel reserves process ID 0, and
		 * occupies process ID 1 for itself.
		 *
		 * To ensure that process ID 0 is never allocated, we fill its
		 * index with garbage. We then expect that should this index
		 * ever be dereferenced we should get a page fault of some kind.
		 **/
		processes.rsrc[0] = (ProcessStream *)0xFEEDBEEF;
		processes.rsrc[PROCID_PROCESS(__KPROCESSID)] = &__kprocess;

		fillOutPrioClasses();
	}

	error_t initialize(void)
	{
		/* DO NOT call __kprocess.initialize() here. It is explicitly
		 * called by __korientationInit().
		 **/
		return ERROR_SUCCESS;
	}

public:
	ContainerProcess *__kgetStream(void) { return &__kprocess; };
	ProcessStream *getStream(processId_t id);
	inline Task *getThread(processId_t tid);

	/**	EXPLANATION:
	 * Distributaries are by nature high privilege processes with high
	 * trust placed in them, and they all run in the kernel executaion
	 * domain. Their scheduler priority and policy are determined by
	 * the kernel and not by the spawning process, and they handle their own
	 * NUMA affinity on a per-thread basis.
	 *
	 * As you can see, the majority of the general userspace arguments
	 * to spawnStream() are redundant for creating distributary processes.
	 **/
	// Callback uses genericCallbackS.
	#define MSGSTREAM_PROCESS_SPAWN_DISTRIBUTARY	(0)
	error_t spawnDistributary(
		utf8Char *commandLine,
		utf8Char *environment,
		numaBankId_t addrSpaceBinding,
		ubit8 schedPrio,
		uarch_t flags,
		void *privateData,
		DistributaryProcess **ret);

	/**	EXPLANATION:
	 * Driver processes can be either kernel processes that share and live
	 * in the kernel's high address space, or normal userspace processes
	 * that live in userspace. Those drivers that are embedded in the
	 * kernel's image are spawned as kernel processes, and all other drivers
	 * are spawned as userspace processes. I.e.: spawnDriver() can return
	 * a self-contained containerProcess, or a containedProcess that lives
	 * in the kernel's address space.
	 *
	 * The "commandLine" argument for spawnDriver() is not the path to a
	 * driver file, but rather the floodplainn VFS path of a device in the
	 * kernel's device tree. You "execute" the device, and the kernel will
	 * find the appropriate driver for it; the user need only know a path
	 * to the device he wants to see brought up.
	 *
	 * There is no need for addrspace binding or CPU affinity arguments
	 * because those things are determined by the NUMA affinity of the
	 * device itself. Privilege elevation for any device is determined by
	 * the kernel and current security policy. There is also no need for
	 * an execDomain argument to this function, because the execDomain of
	 * the driver is decided based on the driver which has been detected
	 * for the device.
	 *
	 * This call will fail if there is no driver detected for the target
	 * device.
	 **/
	#define MSGSTREAM_PROCESS_SPAWN_DRIVER		(2)
	error_t spawnDriver(
		utf8Char *commandLine,
		utf8Char *environment,
		ubit8 prio,
		uarch_t flags,
		void *privateData,
		DriverProcess **ret);

	// Callback uses genericCallbackS.
	#define MSGSTREAM_PROCESS_SPAWN_STREAM		(1)
	error_t spawnStream(
		utf8Char *commandLine,
		utf8Char *environment,
		numaBankId_t addrSpaceBinding,	// NUMA addrspace binding.
		Bitmap *cpuAffinity,		// Ocean/NUMA/SMP affinity.
		void *elevation,		// Privileges.
		ubit8 execDomain,		// Kernel mode vs. User mode.
		ubit8 schedPolicy,		// Sched policy of 1st thread.
		ubit8 prio,			// Sched prio of 1st thread.
		uarch_t flags,			// proc + 1st thread spawn flags
		void *privateData,
		ProcessStream **ret);		// Returned error value.

	error_t destroyStream(void);

private:
	void fillOutPrioClasses(void);
	error_t getNewProcessId(processId_t *ret);

	// All processes begin execution here.
	static void commonEntry(void *);
	static error_t getDistributaryExecutableFormat(
		utf8Char *fullName,
		ProcessStream::executableFormatE *executableFormat,
		void (**entryPoint)(void));

	static error_t getDriverExecutableFormat(
		utf8Char *fullName,
		ProcessStream::executableFormatE *executableFormat,
		void (**entryPoint)(void));

	static error_t getApplicationExecutableFormat(
		utf8Char *fullName,
		ProcessStream::executableFormatE *executableFormat);

	static error_t getExecutableFormat(
		ubit8 *buffer,
		ProcessStream::executableFormatE *executableFormat);

private:
	KernelProcess		__kprocess;
	WrapAroundCounter	nextProcId;
	struct
	{
		void setSlot(processId_t pid, ProcessStream *newProc)
		{
			lock.writeAcquire();
			rsrc[PROCID_PROCESS(pid)] = newProc;
			lock.writeRelease();
		}

		void clearSlot(processId_t pid)
		{
			lock.writeAcquire();
			rsrc[PROCID_PROCESS(pid)] = NULL;
			lock.writeRelease();
		}

		MultipleReaderLock	lock;
		ProcessStream		*rsrc[CHIPSET_MEMORY_MAX_NPROCESSES];
	} processes;
};

extern ProcessTrib	processTrib;

/**	Inline Methods
 *****************************************************************************/

Task *ProcessTrib::getThread(processId_t tid)
{
	ProcessStream		*proc;

	proc = getStream(tid);
	if (proc == NULL) { return NULL; };
	return proc->getTask(tid);
}

#endif

