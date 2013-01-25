
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
	void *entryPoint, taskC::schedPolicyE schedPolicy,
	ubit8 prio, uarch_t flags
	)
{
	sarch_t		threadId;
	taskC		*newTask, *spawningTask;
	error_t		ret;

	// Check for a free thread ID in this process's thread ID space.
	threadId = getNextThreadId();
	if (threadId == -1) { return SPAWNTHREAD_STATUS_NO_PIDS; };

	// Allocate new thread if ID was available.
	newTask = allocateNewThread(this->id | threadId);
	if (newTask == __KNULL) { return ERROR_MEMORY_NOMEM; };

	// Allocate internal sub-structures (context, etc.).
	ret = newTask->initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

	/**	EXPLANATION:
	 * For any thread other than the first, the kernel has a fixed attribute
	 * inheritance model.
	 *
	 * Threads inherit affinity from the thread that spawned them, unless
	 * the spawner passed SPAWNTHREAD_FLAGS_AFFINITY_PINHERIT. That flag
	 * tells the kernel to give the new thread its process global affinity
	 * instead of the affinity of its spawning thread.
	 *
	 * Threads inherit scheduling parameters from the the thread that
	 * spawned them, unless the spawner passes
	 * SPAWNTHREAD_FLAGS_SCHEDPARAMS_SET, which tells the kernel to set the
	 * new thread's scheduling parameters to those passed in the syscall.
	 *
	 * When the thread being spawned is the first in the process, the kernel
	 * uses a different inheritance model.
	 *
	 * The first thread in a process inherits its affinity attributes from
	 * its process stream unconditionally. There is no modifier to change
	 * the way that the fist thread in a process inherits affinity.
	 * This is because in the syscall for process spawning, there are
	 * arguments which allow for inheritance modification of the spawned
	 * process. Therefore the first thread in a process must follow those
	 * passed modifiers.
	 *
	 * The first thread in a process will be set to the scheduling 
	 * parameters passed in the syscall that created the process. If none
	 * were passed, the scheduling parameters will default to system
	 * defaults.
	 **/

	spawningTask = cpuTrib.getCurrentCpuStream()->taskStream.currentTask;
	if (__KFLAG_TEST(flags, SPAWNTHREAD_FLAGS_FIRST_THREAD))
	{
		ret = initializeFirstThread(
			newTask, spawningTask, schedPolicy, prio, flags);
	}
	else
	{
		ret = initializeChildThread(
			newTask, spawningTask, schedPolicy, prio, flags);
	};
	if (ret != ERROR_SUCCESS) { return ret; };

	// New task has inherited as needed. Initialize register context.
	taskContext::initialize(&newTask->context, newTask, entryPoint);
	return taskTrib.schedule(newTask);
}

taskC *processStreamC::allocateNewThread(processId_t id)
{
	taskC		*ret;
 
	ret = new taskC(id, this, SCHEDPRIO_DEFAULT, TASK_FLAGS_CUSTPRIO);
	if (ret == __KNULL) { return __KNULL; };

	// Add the new thread to the process's task array.
	tasks[PROCID_THREAD(id)] = ret;
	return ret;
}

void processStreamC::removeThread(processId_t id)
{
	if (tasks[PROCID_THREAD(id)] != __KNULL)
	{
		delete tasks[PROCID_THREAD(id)];
		tasks[PROCID_THREAD(id)] = __KNULL;
	};
}

