#ifndef _PROCESS_H
	#define _PROCESS_H

	#include <scaling.h>
	#include <arch/memory.h>
	#include <chipset/memory.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kflagManipulation.h>
	#include <__kclasses/bitmap.h>
	#include <__kclasses/wrapAroundCounter.h>
	#include <kernel/common/task.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/multipleReaderLock.h>
	#include <kernel/common/execDomain.h>
	#include <kernel/common/memoryTrib/memoryStream.h>
	#include <kernel/common/timerTrib/timerStream.h>
	#include <kernel/common/floodplainn/floodplainnStream.h>
	#include <__kthreads/__korientation.h>

#define PROCESS_ENV_MAX_NVARS		(64)
#define PROCESS_ENV_NAME_MAXLEN		(96)
#define PROCESS_ENV_VALUE_MAXLEN	(512)
#define PROCESS_FULLNAME_MAXLEN		(2000)
#define PROCESS_ARGUMENTS_MAXLEN	(32700)

/**	Flags for processC::flags.
 **/
#define PROCESS_FLAGS___KPROCESS	(1<<0)

/**	Flags for processStreamC::spawnThread().
 **/
// STINHERIT by default.
#define SPAWNTHREAD_FLAGS_AFFINITY_STINHERIT	(0)
// Implied if the cpuAffinity BMP != __KNULL && !PINHERIT
#define SPAWNTHREAD_FLAGS_AFFINITY_SET		(0)
#define SPAWNTHREAD_FLAGS_AFFINITY_PINHERIT	(1<<1)

// STINHERIT by default.
#define SPAWNTHREAD_FLAGS_SCHEDPOLICY_STINHERIT	(0)
#define SPAWNTHREAD_FLAGS_SCHEDPOLICY_SET	(1<<3)

// Set to PRIOCLASS_DEFAULT by default.
#define SPAWNTHREAD_FLAGS_SCHEDPRIO_CLASS_DEFAULT	(0)
#define SPAWNTHREAD_FLAGS_SCHEDPRIO_STINHERIT		(1<<7)
#define SPAWNTHREAD_FLAGS_SCHEDPRIO_PRIOCLASS_SET	(1<<5)
#define SPAWNTHREAD_FLAGS_SCHEDPRIO_SET			(1<<6)

// Set to prevent the thread from being scheduled automatically on creation.
#define SPAWNTHREAD_FLAGS_DORMANT			(1<<8)

#define SPAWNTHREAD_STATUS_NO_PIDS		0x1

class processStreamC
{
public:
	processStreamC(
		processId_t processId, processId_t parentProcId,
		ubit8 execDomain)
	:
		id(processId << PROCID_PROCESS_SHIFT),
		parentId(parentProcId << PROCID_PROCESS_SHIFT),
		flags(0),

		// Kernel process hands out thread IDs from 1 since 0 is taken.
		nextTaskId(
			CHIPSET_MEMORY_MAX_NTASKS - 1,
			(processId == __KPROCESSID) ? 1 : 0),
		nTasks(0),

		fullName(__KNULL), workingDirectory(__KNULL),
		arguments(__KNULL),
		nEnvVars(0),
		environment(__KNULL),

		execDomain(execDomain),

		memoryStream(processId, this),
		timerStream(processId, this),
		floodplainnStream(processId, this)
	{
		memset(tasks, 0, sizeof(tasks));
		defaultMemoryBank.rsrc = NUMABANKID_INVALID;

		if (id == __KPROCESSID)
		{
			__KFLAG_SET(flags, PROCESS_FLAGS___KPROCESS);
			nTasks = 1;
			tasks[0] = &__korientationThread;
			defaultMemoryBank.rsrc =
				CHIPSET_MEMORY_NUMA___KSPACE_BANKID;
		};
	}

	error_t initialize(
		const utf8Char *commandLine, const utf8Char *environment,
		bitmapC *cpuAffinity);

	~processStreamC(void);

public:
	taskC *getTask(processId_t processId);
	taskC *getThread(processId_t processId) { return getTask(processId); }

	ubit32 getProcessFullNameMaxLength(void)
		{ return PROCESS_FULLNAME_MAXLEN; }

	ubit32 getProcessArgumentsMaxLength(void)
		{ return PROCESS_ARGUMENTS_MAXLEN; }

	virtual vaddrSpaceStreamC *getVaddrSpaceStream(void)=0;

	// I am very reluctant to have this "argument" parameter.
	error_t spawnThread(
		void (*entryPoint)(void *),
		void *argument,
		bitmapC *cpuAffinity,
		taskC::schedPolicyE schedPolicy, ubit8 prio,
		uarch_t flags,
		processId_t *ret);

public:
	error_t cloneStateIntoChild(processStreamC *child);
	error_t initializeFirstThread(
		taskC *task, taskC *spawningThread,
		taskC::schedPolicyE policy, ubit8 prio, uarch_t flags);

	error_t initializeChildThread(
		taskC *task, taskC *spawningThread,
		taskC::schedPolicyE policy, ubit8 prio, uarch_t flags);

public:
	struct environmentVarS
	{
		utf8Char	name[PROCESS_ENV_NAME_MAXLEN],
				value[PROCESS_ENV_VALUE_MAXLEN];
	};

	processId_t		id, parentId;
	uarch_t			flags;

	multipleReaderLockC	taskLock;
	wrapAroundCounterC	nextTaskId;
	uarch_t			nTasks;
	taskC			*tasks[CHIPSET_MEMORY_MAX_NTASKS];

	utf8Char		*fullName, *workingDirectory, *arguments;
	ubit8			nEnvVars;
	environmentVarS		*environment;

	// Execution domain and privilege elevation information.
	ubit8			execDomain;

#if __SCALING__ >= SCALING_SMP
	// Tells us which CPUs this process has run on.
	bitmapC			cpuTrace;
	bitmapC			cpuAffinity;
#endif
#if __SCALING__ >= SCALING_CC_NUMA
	sharedResourceGroupC<multipleReaderLockC, numaBankId_t>
		defaultMemoryBank;
#endif

	// Events which have been queued on this process.
	bitmapC			pendingEvents;

	memoryStreamC		memoryStream;
	timerStreamC		timerStream;
	floodplainnStreamC	floodplainnStream;

private:
	sarch_t getNextThreadId(void);
	taskC *allocateNewThread(processId_t newThreadId);
	void removeThread(processId_t id);

	void __kprocessGenerateEnvString(utf8Char *);
	void __kprocessInitializeBmps(void);

	error_t allocateInternals(void);
	error_t generateFullName(
		const utf8Char *commandLineString, ubit16 *argumentsStartIndex);

	error_t generateArguments(const utf8Char *argumentString);
	error_t generateEnvironment(const utf8Char *environmentString);
	error_t initializeBmps(void);
};

/**	ContainerProcessC and containedProcessC.
 ******************************************************************************/

class containerProcessC
:
public processStreamC
{
public:
	containerProcessC(
		processId_t processId, processId_t parentProcessId,
		ubit8 execDomain, numaBankId_t numaAddrSpaceBinding,
		void *vaddrSpaceBaseAddr, uarch_t vaddrSpaceSize)
	:
	processStreamC(processId, parentProcessId, execDomain),
	addrSpaceBinding(numaAddrSpaceBinding),
	vaddrSpaceStream(
		processId, this,
		vaddrSpaceBaseAddr, vaddrSpaceSize)
	{}

	error_t initialize(
		const utf8Char *commandLine, const utf8Char *environment,
		bitmapC *affinity)
	{
		error_t		ret;

		ret = processStreamC::initialize(
			commandLine, environment, affinity);

		if (ret != ERROR_SUCCESS) { return ret; };
		return vaddrSpaceStream.initialize();
	}

	~containerProcessC(void) {}

public:
	virtual vaddrSpaceStreamC *getVaddrSpaceStream(void)
		{ return &vaddrSpaceStream; }

public:
	numaBankId_t		addrSpaceBinding;

private:
	vaddrSpaceStreamC	vaddrSpaceStream;
};

class containedProcessC
:
public processStreamC
{
public:
	containedProcessC(
		processId_t processId, processId_t parentProcessId,
		ubit8 execDomain,
		containerProcessC *containerProcess)
	:
	processStreamC(processId, parentProcessId, execDomain),
	containerProcess(containerProcess)
	{}

	error_t initialize(
		const utf8Char *commandLine, const utf8Char *environment,
		bitmapC *affinity)
	{
		return processStreamC::initialize(
			commandLine, environment,affinity);
	}

	~containedProcessC(void) {}

public:
	containerProcessC *getContainerProcess(void)
		{ return containerProcess; }

	virtual vaddrSpaceStreamC *getVaddrSpaceStream(void)
		{ return containerProcess->getVaddrSpaceStream(); }

private:
	containerProcessC	*containerProcess;
};

/**	distributaryProcessC
 ******************************************************************************/

class distributaryProcessC
:
public containerProcessC
{
public:
	distributaryProcessC(
		processId_t processId, processId_t parentProcessId,
		numaBankId_t numaAddrSpaceBinding)
	:
	containerProcessC(
		processId, parentProcessId,
		PROCESS_EXECDOMAIN_KERNEL,	// Always kernel domain.
		numaAddrSpaceBinding,
		(void *)0x100000, ARCH_MEMORY___KLOAD_VADDR_BASE - 0x1000)
	{}

	error_t initialize(
		const utf8Char *fullName, const utf8Char *environment,
		bitmapC *affinity)
	{
		return containerProcessC::initialize(
			fullName, environment, affinity);
	}

	~distributaryProcessC(void) {}
};

/**	containerDriverProcessC, driverProcessC (contained),
 *	userspaceProcessC, containedUserspaceProcessC (contained).
 **/

/**	Inline Methods:
 *****************************************************************************/

inline taskC *processStreamC::getTask(processId_t id)
{
	taskC		*ret;
	uarch_t		rwFlags;

	taskLock.readAcquire(&rwFlags);
	ret = tasks[PROCID_TASK(id)];
	taskLock.readRelease(rwFlags);

	return ret;
}

inline sarch_t processStreamC::getNextThreadId(void)
{
	return nextTaskId.getNextValue(reinterpret_cast<void **>( tasks ));
}

#endif

