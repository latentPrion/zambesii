
#include <chipset/memory.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/timerTrib/timerTrib.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/taskTrib/taskTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/vfsTrib/vfsTrib.h>
#include <__kthreads/__korientation.h>


prioClassS	prioClasses[PRIOCLASS_NCLASSES];

void processTribC::fillOutPrioClasses(void)
{
	new (&prioClasses[0]) prioClassS(CC"Idle", 0);
	new (&prioClasses[1]) prioClassS(CC"Low", 4);
	new (&prioClasses[2]) prioClassS(CC"Moderate", 8);
	new (&prioClasses[3]) prioClassS(CC"High", 12);
	new (&prioClasses[4]) prioClassS(CC"Critical", 16);
}

static inline error_t resizeAndMergeBitmaps(bitmapC *dest, bitmapC *src)
{
	error_t		ret;

	ret = dest->resizeTo(src->getNBits());
	if (ret != ERROR_SUCCESS) { return ret; };

	dest->merge(src);
	return ERROR_SUCCESS;
}

error_t processTribC::getDistributaryExecutableFormat(
	utf8Char *fullName, processStreamC::executableFormatE *executableFormat,
	void (**entryPoint)(void)
	)
{
	vfs::inodeTypeE			inodeType;
	void				*tag;
	error_t				ret;
	taskC				*self;

	/**	EXPLANATION:
	 * For Distributaries, there are IN_KERNEL and OUT_OF_KERNEL dtribs.
	 * IN_KERNEL dtribs will always be linked directly into the kernel image
	 * and are basically like raw binaries embedded and fully linked into
	 * the kernel.
	 *
	 * OUT_OF_KERNEL distributaries are those which are loaded after boot
	 * time and are actually stored on disk, and advertised to the kernel
	 * via syscalls.
	 *
	 * This function checks to see if this process is an IN_KERNEL
	 * distributary or not, and if it is, it will return executable format
	 * RAW.
	 *
	 * If it's OUT_OF_KERNEL, this function will read the file path
	 * of the real file on disk, and pass that on to getExecutableFormat(),
	 * which will then load the first few bytes of that file and determine
	 * its executable format.
	 **/
	self = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentTask();

	ret = vfsTrib.getDvfs()->getPath(fullName, &inodeType, &tag);

	if (ret != ERROR_SUCCESS)
	{
		__kprintf(ERROR"Proc 0x%x: command line invalid; "
			"distributary doesn't exist.\n",
			self->getFullId());

		return SPAWNPROC_STATUS_INVALID_FILE_NAME;
	};

	if (inodeType == vfs::DIR)
	{
		tag = ((dvfs::categoryTagC *)tag)->getInode()
			->getLeafTag(CC"default");

		if (tag == __KNULL)
		{
			__kprintf(ERROR"Proc 0x%x: command line "
				"invalid; disributary doesn't exist.\n",
				self->getFullId());

			return SPAWNPROC_STATUS_INVALID_FILE_NAME;
		};
	};

	if (((dvfs::distributaryTagC *)tag)->getInode()->getType()
		== dvfs::distributaryInodeC::IN_KERNEL)
	{
		*entryPoint = ((dvfs::distributaryTagC *)tag)->getInode()
			->getEntryAddress();

		*executableFormat = processStreamC::RAW;
		return ERROR_SUCCESS;
	}
	else
	{
		__kprintf(FATAL"Proc 0x%x: out-of-kernel dtribs are "
			"not yet supported.\n",
			self->getFullId());

		ret = getExecutableFormat(
			((dvfs::distributaryTagC *)tag)->getInode()
				->getFullName(),
			executableFormat);

		if (ret != ERROR_SUCCESS) { return ret; };

		// OUT_OF_KERNEL distributaries can only be ELF.
		if (*executableFormat != processStreamC::ELF) {
			return ERROR_UNSUPPORTED;
		};

		return ERROR_UNSUPPORTED;
	};
}

error_t processTribC::getExecutableFormat(
	utf8Char *, processStreamC::executableFormatE *
	)
{
	/* This function reads a file from disk and determines its executable
	 * file format.
	 *
	 * All of the other get*ExecutableFormat() functions will usually
	 * eventually call down into this one, though there are exceptions;
	 * two exceptions at most.
	 *
	 * The file fullname passed as the first argument is the fullname of the
	 * file to be examined.
	 **/
	return ERROR_UNSUPPORTED;
}

/**	EXPLANATION:
 * This is the common entry point for every process other than the kernel
 * process. It seeks to find out what executable format the new process' image
 * data is stored in, and load the correct syslib for that process based on its
 * executable format.
 *
 * Zambesii does not prefer any particular executable format, but its native
 * format is ELF. Confluence libs can be made in any format and the kernel will
 * load them all. Basically for an ELF process, the kernel will seek to load
 * an ELF confluence lib, for a PE process, the kernel will seek to load a PE
 * confluence lib, etc.
 *
 * These libraries need not be part of the kernel, and can be distributed in
 * userspace. If the kernel fails to guess what executable format the new
 * process' image is encoded in, it will simply refuse to load the process
 * ("file format not recognized").
 *
 * An exception to this rule is in-kernel distributaries; that is,
 * distributaries that are embedded in the kernel image and require no
 * executable format parsing -- these types of distributary process will just
 * be executed raw.
 **/
void processTribC::commonEntry(void *)
{
	taskC						*self;
	processStreamC::initializationBlockSizeInfoS	initBlockSizes;
	processStreamC::initializationBlockS		*initBlock;
	processStreamC::executableFormatE		executableFormat;
	uarch_t						initBlockNPages;
	error_t						err;
	void						(*jumpAddress)(void);

	/**	EXPLANATION:
	 * Purpose of this common entry routine is to detect the executable
	 * format of the new process and load the correct confluence library
	 * for it. This routine /does not/ jump into the new process and begin
	 * its execution.
	 *
	 * Rather it loads a fixed, predetermined confluence library and jumps
	 * to that instead. That confluence library contains the code to load
	 * the new process and jump to it.
	 *
	 * Only in the case of an IN_KERNEL distributary does this function
	 * jump and begin the new process immediately.
	 **/
	jumpAddress = __KNULL;
	self = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentTask();

	__kprintf(NOTICE"New process running. ID = 0x%x.\n",
		self->getFullId());

	self->parent->getInitializationBlockSizeInfo(&initBlockSizes);
	initBlockNPages = PAGING_BYTES_TO_PAGES(
		sizeof(processStreamC::initializationBlockS)
			+ initBlockSizes.fullNameSize + 1
			+ initBlockSizes.workingDirectorySize + 1
			+ initBlockSizes.argumentsSize + 1);

	initBlock = new (
		self->parent->memoryStream.memAlloc(initBlockNPages))
		processStreamC::initializationBlockS;

	initBlock->fullName = (utf8Char *)&initBlock[1];
	initBlock->workingDirectory =
		(utf8Char *)((uintptr_t)initBlock->fullName
			+ initBlockSizes.fullNameSize);

	initBlock->arguments =
		(utf8Char *)((uintptr_t)initBlock->workingDirectory
			+ initBlockSizes.argumentsSize);

	self->parent->getInitializationBlock(initBlock);

	/*__kprintf(NOTICE"Proc 0x%x: initBlock %d pages, mem 0x%p.\n"
		"\tfullNameSize %d, workingDirSize %d, argumentsSize %d.\n",
		self->getFullId(), initBlockNPages, initBlock,
		initBlockSizes.fullNameSize,
		initBlockSizes.workingDirectorySize,
		initBlockSizes.argumentsSize);

	__kprintf(NOTICE"Proc 0x%x: command line: \"%s\".\n",
		self->getFullId(), initBlock->fullName);*/

	switch (initBlock->type)
	{
	case (ubit8)processStreamC::DISTRIBUTARY:
		err = getDistributaryExecutableFormat(
			initBlock->fullName, &executableFormat, &jumpAddress);

		break;

	default:
		__kprintf(FATAL"Proc 0x%x: Currently unsupported process type "
			"%d.\n",
			self->getFullId(), initBlock->type);

		err = ERROR_UNSUPPORTED;
		break;
	};

	if (err != ERROR_SUCCESS)
	{
		__kprintf(FATAL"Proc 0x%x: Error identifying executable "
			"format. Halting.\n",
			self->getFullId());

		for (;;) { asm volatile("hlt\n\t"); };
	};

	if (executableFormat != processStreamC::RAW)
	{
		// Load the confluence lib and extract its entry point addr.
		__kprintf(FATAL"Proc 0x%x: Currently unsupported executable "
			"format %d.\n",
			self->getFullId(), executableFormat);

		for (;;) { asm volatile("hlt\n\t"); };
	};

	/* For RAW processes, jumpAddress is set by their get*ExecutableFormat.
	 * Time to jump to the next stage. For everything other than RAW, we
	 * jump to a confluence library that holds a parser for the process'
	 * executable format. For RAW, we jump directly into the process.
	 *
	 * The location we are about to jump to is whatever is held inside of
	 * "jumpAddress". This would have been set either the process'
	 * get*ExecutableFormat() function, or by the confluence loading code.
	 *
	 * Right now we are executing on the new process' kernel stack.
	 * We can construct the new register context on its userspace
	 * stack if we wish; this is simpler as well since there is no
	 * risk of trampling.
	 **/
	taskContextC		*context;

	context = (taskContextC *)((uintptr_t)self->stack1
		+ (CHIPSET_MEMORY_USERSTACK_NPAGES * PAGING_BASE_SIZE));

	context--;
	new (context) taskContextC(self->parent->execDomain);
	err = context->initialize();
	if (err != ERROR_SUCCESS) { for (;;) { asm volatile("hlt\n\t"); }; };

	// Using the userspace stack.
	context->setStacks(self->stack0, self->stack1, 1);
	context->setEntryPoint((void (*)(void *))jumpAddress);

	// Now just jump.
	loadContextAndJump(context);
}

error_t processTribC::spawnDistributary(
	utf8Char *commandLine,
	utf8Char *environment,
	numaBankId_t addrSpaceBinding,
	ubit8 /*schedPrio*/,
	uarch_t /*flags*/,
	distributaryProcessC **newProcess
	)
{
	error_t			ret;
	processId_t		newProcessId;
	processStreamC		*parentProcess;
	taskC			*firstTask;

	if (commandLine == __KNULL || newProcess == __KNULL)
		{ return ERROR_INVALID_ARG; };

	parentProcess = cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentTask()->parent;

	ret = getNewProcessId(&newProcessId);
	if (ret != ERROR_SUCCESS)
	{
		__kprintf(NOTICE PROCTRIB"Out of process IDs.\n");
		return ret;
	};

	*newProcess = new distributaryProcessC(
		newProcessId, parentProcess->id, addrSpaceBinding);

	if (*newProcess == __KNULL)
	{
		__kprintf(NOTICE PROCTRIB"Failed to alloc dtrib process.\n");
		return ERROR_MEMORY_NOMEM;
	};

	bitmapC		affinity;

	ret = affinity.initialize(cpuTrib.onlineCpus.getNBits());
	if (ret != ERROR_SUCCESS) { return ret; };
	affinity.merge(&cpuTrib.onlineCpus);

	ret = (*newProcess)->initialize(commandLine, environment, &affinity);
	if (ret != ERROR_SUCCESS) { return ret; };

	processes.lock.writeAcquire();
	processes.rsrc[newProcessId] = *newProcess;
	processes.lock.writeRelease();

	// Spawn the first thread.
	ret = (*newProcess)->spawnThread(
		&processTribC::commonEntry, __KNULL,
		&(*newProcess)->cpuAffinity,
		taskC::ROUND_ROBIN, 0,
		SPAWNTHREAD_FLAGS_AFFINITY_SET
		| SPAWNTHREAD_FLAGS_SCHEDPOLICY_SET
		| SPAWNTHREAD_FLAGS_FIRST_THREAD,
		&firstTask);

	if (ret != ERROR_SUCCESS)
	{
		__kprintf(NOTICE"Failed to spawn thread for new process.\n");
		return ret;
	};

	__kprintf(NOTICE"New process spawned, tid = 0x%x.\n",
		firstTask->getFullId());

	return ERROR_SUCCESS;
}

#if 0
error_t *processTribC::spawnStream(
	numaBankId_t,				// NUMA addrspace binding.
	bitmapC *cpuAffinity,			// Ocean/NUMA/SMP affinity.
	void */*elevation*/,			// Privileges.
	ubit8 execDomain,			// Kernel mode vs. User mode.
	uarch_t flags,				// Process spawn flags.
	ubit8 /*schedPolicy*/,			// Sched policy of 1st thread.
	ubit8 /*prio*/,				// Sched prio of 1st thread.
	uarch_t /*ftFlags*/,			// 1st thread spawn flags.
	error_t *err				// Returned error value.
	)
{
	processId_t		newId, parentId;
	sbit32			newIdTmp;
	uarch_t			rwFlags;
	processStreamC		*newProc=__KNULL;
	utf8Char		*fileName, *workingDir;
	/**	NOTES:
	 * This routine will essentially be the guiding hand to starting up
	 * process: finding the file in the VFS, then finding out what
	 * executable format it is encoded in, then calling the exec format
	 * parser to get an exec map, and do dynamic linking, then spawning
	 * all of its streams (Memory Stream, etc), then calling the Task Trib
	 * to spawn its first thread.
	 **/
	// Get a file descriptor for the executable.

	/* ...Eventually, you'll end up with a vfsFileC descriptor. Next stage
	 * will be to try to get a parser for the file, so you can know whether
	 * or not it's executable.
	 **/

	/* If you manage to get a parser, the next step would be to get a list
	 * all its dynamic dependencies.
	 *
	 * Assuming you get all of those, now you can allocate and initialize
	 * a new processC object for this new process and initialize its
	 * address space.
	 *
	 * Next, copy the executable image into the address space and relocate
	 * it and all libraries. Spawn a thread 0 with the entry point of the
	 * process as its entry point. That's all.
	 **/

	// Get process ID from counter.
	processes.lock.readAcquire(&rwFlags);
	newIdTmp = nextProcId.getNextValue(
		reinterpret_cast<void **>( processes.rsrc ));

	processes.lock.readRelease(rwFlags);

	if (newIdTmp < 0)
	{
		__kprintf(WARNING PROCTRIB"Out of process IDs.\n");
		*err = ERROR_GENERAL;
		return __KNULL;
	};

	newId = newIdTmp << PROCID_PROCESS_SHIFT;
	// Get parent (spawning) process' process ID.
	parentId = cpuTrib.getCurrentCpuStream()
		->taskStream.getCurrentTask()->parent->id;

	// Call initialize().
	*err = newProc->initialize(_commandLine, fileName, workingDir);
	if (*err != ERROR_SUCCESS)
	{
		delete newProc;
		return __KNULL;
	};

	// Add the new process to the global process array.
	processes.lock.writeAcquire();
	processes.rsrc[PROCID_PROCESS(newId)] = newProc;
	processes.lock.writeRelease();

	// Assign exec domain.
	newProc->execDomain = execDomain;

	// Deal with affinity for the new process.
	if (__KFLAG_TEST(flags, PROCESSTRIB_SPAWN_FLAGS_PINHERIT_AFFINITY))
	{
		// Inherit affinity from spawning process.
		*err = resizeAndMergeBitmaps(
			&newProc->cpuAffinity,
			&cpuTrib.getCurrentCpuStream()
				->taskStream.getCurrentTask()->parent->cpuAffinity);
	}
	else if (__KFLAG_TEST(flags, PROCESSTRIB_SPAWN_FLAGS_STINHERIT_AFFINITY))
	{
		// Inherit affinity from spawning thread.
		*err = resizeAndMergeBitmaps(
			&newProc->cpuAffinity,
			&cpuTrib.getCurrentCpuStream()
				->taskStream.getCurrentTask()->cpuAffinity);
	}
	else
	{
		// If affinity argument is valid:
		if (cpuAffinity != __KNULL)
		{
			*err = resizeAndMergeBitmaps(
				&newProc->cpuAffinity, cpuAffinity);
		}
		else
		{
			/* Else pull "default" affinity of "all banks/cpus" from
			 * Memory trib and CPU trib.
			 **/
			// Size up and copy bitmaps: Online CPUs.
			*err = resizeAndMergeBitmaps(
				&newProc->cpuAffinity, &cpuTrib.onlineCpus);

			if (*err != ERROR_SUCCESS) { return __KNULL; };
		};
	};

	// TODO: Handle elevation.

	// 
	return newProc;
}
#endif

