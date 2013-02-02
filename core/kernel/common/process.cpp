
#include <chipset/chipset.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kstdlib/__kcxxlib/new>
#include <kernel/common/process.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/vfsTrib/vfsTrib.h>
#include <kernel/common/taskTrib/taskTrib.h>


// Max length for the kernel command line args.
#define __KPROCESS_MAX_NARGS			(16)
#define __KPROCESS_ARG_MAX_NCHARS		(32)
#define __KPROCESS_STRING_MAX_NCHARS		(64)

// Preallocated mem space for the kernel command line args.
utf8Char	*__kprocessArgPointerMem[__KPROCESS_MAX_NARGS];
utf8Char	__kprocessArgStringMem
	[__KPROCESS_MAX_NARGS]
	[__KPROCESS_ARG_MAX_NCHARS];

/**	EXPLANATION:
 * Pre-allocated memory for the strings in the kernel process object. Namely:
 *	fullName, workingDir, fileName.
 **/
utf8Char	__kprocessPreallocatedStringMem
	[3]
	[__KPROCESS_STRING_MAX_NCHARS];


#if __SCALING__ >= SCALING_SMP
// Preallocated mem space for the internals of the __kprocess bitmapC objects.
ubit8		__kprocessPreallocatedBmpMem[2][32];
#endif

error_t processStreamC::initialize(
	const utf8Char * /*commandLine*/, bitmapC * /*cpuAffinity*/
	)
{
	//error_t		ret;

	// The kernel process is initialized differently.
	if (PROCESSID_PROCESS(id) == __KPROCESSID)
	{
		__kprocessAllocateInternals();
		// __kprocessGenerateArgString();
		// __kprocessGenerateEnvString();
		__kprocessInitializeBmps();
	}
	else
	{
		/*ret = allocateInternals();
		if (ret != ERROR_SUCCESS) { return ret; };
		ret = generateArgString();
		if (ret != ERROR_SUCCESS) { return ret; };
		ret = generateEnvString();
		if (ret != ERROR_SUCCESS) { return ret; };
		ret = initializeBmps();
		if (ret != ERROR_SUCCESS) { return ret; };*/
	};

	// Initialize internal bitmaps.

	// Fill out the fullName, workingDirectory and fileName strings.

	return ERROR_SUCCESS;
}

void processStreamC::__kprocessAllocateInternals(void)
{
	// Assign our preallocated mem to the dynamic members.
	fullName = &__kprocessPreallocatedStringMem[0][0];
	workingDirectory = &__kprocessPreallocatedStringMem[1][0];
	fileName = &__kprocessPreallocatedStringMem[2][0];
}

void processStreamC::__kprocessInitializeBmps(void)
{
#if __SCALING__ >= SCALING_SMP
	cpuTrace.initialize(0, &__kprocessPreallocatedBmpMem[0], 32);
	cpuTrace.initialize(0, &__kprocessPreallocatedBmpMem[1], 32);
#endif
}

processStreamC::~processStreamC(void)
{
}

static inline error_t resizeAndMergeBitmaps(bitmapC *dest, bitmapC *src)
{
	error_t		ret;

	ret = dest->resizeTo(src->getNBits());
	if (ret != ERROR_SUCCESS) { return ret; };

	dest->merge(src);
	return ERROR_SUCCESS;
}

error_t processStreamC::cloneStateIntoChild(processStreamC *child)
{
	error_t		ret;
//	ubit32		len;

/*	len = strlen8(env);
	child->env = new utf8Char[len + 1];
	if (child->env == __KNULL) { return ERROR_MEMORY_NOMEM; };
	strcpy8(child->env, env); */

	ret = resizeAndMergeBitmaps(&child->cpuAffinity, &cpuAffinity);
	if (ret != ERROR_SUCCESS) { return ret; };
	// child->localAffinity = affinity::findLocal(&child->affinity);

	child->execDomain = execDomain;
	return ERROR_SUCCESS;
}

error_t processStreamC::spawnThread(
	void (* entryPoint)(void *), void * /*argument*/,
	bitmapC * cpuAffinity,
	taskC::schedPolicyE schedPolicy,
	ubit8 prio, uarch_t flags,
	processId_t *newThreadId
	)
{
	taskC		*newTask;
	error_t		ret;
	sarch_t		newThreadIdTmp;

	// Check for a free thread ID in this process's thread ID space.
	newThreadIdTmp = getNextThreadId();
	if (newThreadIdTmp == -1) { return SPAWNTHREAD_STATUS_NO_PIDS; };
	// Allocate new thread if ID was available.
	*newThreadId = newThreadIdTmp;
	newTask = allocateNewThread(*newThreadId);
	if (newTask == __KNULL) { return ERROR_MEMORY_NOMEM; };

	// Allocate internal sub-structures (context, etc.).
	ret = newTask->initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

	ret = newTask->allocateStacks();
	if (ret != ERROR_SUCCESS) { return ret; };

#if __SCALING__ >= SCALING_SMP
	// Do affinity inheritance.
	ret = newTask->inheritAffinity(cpuAffinity, flags);
	if (ret != ERROR_SUCCESS) { return ret; };
#endif

	newTask->inheritSchedPolicy(schedPolicy, flags);
	newTask->inheritSchedPrio(prio, flags);

	// Now everything is allocated; just initialize the new thread.
	newTask->context->setStacks(
		execDomain, newTask->stack0, newTask->stack1);

	newTask->context->setEntryPoint(entryPoint);

__kprintf(NOTICE"New task: cs %d, eip 0x%p, esp 0x%p, ss %d, ds %d.\n",
	newTask->context->cs,
	newTask->context->eip,
	newTask->context->esp,
	newTask->context->ss,
	newTask->context->ds);

	return taskTrib.schedule(newTask);
}

taskC *processStreamC::allocateNewThread(processId_t newThreadId
	)
{
	taskC		*ret;

	ret = new taskC(PROCID_PROCESS(id) | newThreadId, this);
	if (ret == __KNULL) { return __KNULL; };

	taskLock.writeAcquire();

	// Add the new thread to the process's task array.
	tasks[PROCID_THREAD(id)] = ret;
	nTasks++;

	taskLock.writeRelease();
	return ret;
}

void processStreamC::removeThread(processId_t id)
{
	taskLock.writeAcquire();

	if (tasks[PROCID_THREAD(id)] != __KNULL)
	{
		delete tasks[PROCID_THREAD(id)];
		tasks[PROCID_THREAD(id)] = __KNULL;
	};

	taskLock.writeRelease();
}

