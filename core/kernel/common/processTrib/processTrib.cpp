
#include <chipset/memory.h>
#include <__kstdlib/__kcxxlib/memory>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/panic.h>
#include <kernel/common/messageStream.h>
#include <kernel/common/timerTrib/timerTrib.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/taskTrib/taskTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/vfsTrib/vfsTrib.h>
#include <kernel/common/floodplainn/floodplainn.h>
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
	dvfs::tagC			*tag;
	error_t				ret;
	threadC				*self;

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
	*executableFormat = processStreamC::RAW;
	self = (threadC *)cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentTask();

	ret = vfsTrib.getDvfs()->getPath(fullName, &tag);

	if (ret != ERROR_SUCCESS)
	{
		printf(ERROR"Proc 0x%x: command line invalid; "
			"distributary doesn't exist.\n",
			self->getFullId());

		return SPAWNPROC_STATUS_INVALID_FILE_NAME;
	};

	if (tag->getDInode()->getType() == dvfs::distributaryInodeC::IN_KERNEL)
	{
		// IN_KERNEL dtribs are raw embedded code in the kernel image.
		*entryPoint = tag->getDInode()->getEntryAddress();
	}
	else
	{
		// OUT_OF_KERNEL distributaries can only be ELF.
		*executableFormat = processStreamC::ELF;
		printf(WARNING"Proc 0x%x: out-of-kernel dtribs are "
			"not yet supported.\n",
			self->getFullId());
	};

	return ERROR_SUCCESS;
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

static ubit32 getCallbackFunctionNo(threadC *self)
{
	switch (self->parent->getType())
	{
	case processStreamC::DISTRIBUTARY:
		return MSGSTREAM_PROCESS_SPAWN_DISTRIBUTARY;
	case processStreamC::KERNEL_DRIVER:
		return MSGSTREAM_PROCESS_SPAWN_DRIVER;
	default:
		return MSGSTREAM_PROCESS_SPAWN_STREAM;
	};
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
 * confluence lib, and so on.
 *
 * These libraries need not be part of the kernel, and can be distributed in
 * userspace. If the kernel fails to guess what executable format the new
 * process' image is encoded in, it will simply refuse to load the process
 * ("file format not recognized").
 *
 * Exceptions to this rule are in-kernel distributaries and in-kernel drivers;
 * these are embedded in the kernel image and require no executable format
 * parsing -- these types of process images will just be executed raw, albeit
 * in the kernel-domain.
 **/
void processTribC::commonEntry(void *)
{
	threadC						*self;
	processStreamC::initializationBlockSizeInfoS	initBlockSizes;
	processStreamC::initializationBlockS		*initBlock;
	processStreamC::executableFormatE		executableFormat;
	messageStreamC::headerS				*callbackMessage;
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
	jumpAddress = NULL;
	self = (threadC *)cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentTask();

	printf(NOTICE"New process running. ID=0x%x type=%d.\n",
		self->getFullId(),
		self->parent->getType());

	// Allocate the callback message memory.
	callbackMessage = new messageStreamC::headerS(
		self->parent->parentThreadId,
		MSGSTREAM_SUBSYSTEM_PROCESS, getCallbackFunctionNo(self),
		sizeof(*callbackMessage), 0, self->parent->privateData);

	if (callbackMessage == NULL)
	{
		printf(FATAL PROCTRIB"commonEntry: process 0x%x:\n",
			self->getFullId());

		panic(CC"\tFailed to allocate callback message.\n");
	};

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

	/* Get the executable format for the new process. If it's a RAW code
	 * sequence that is embedded in the kernel image, we also obtain the
	 * jumpAddress right here; else the jumpAddress is determined later on
	 **/
	switch (initBlock->type)
	{
	case (ubit8)processStreamC::DISTRIBUTARY:
		err = getDistributaryExecutableFormat(
			initBlock->fullName, &executableFormat, &jumpAddress);

		break;

	case (ubit8)processStreamC::KERNEL_DRIVER:
		// Common kernel driver entry point is provided by Floodplainn.
		err = ERROR_SUCCESS;
		executableFormat = processStreamC::RAW;
		jumpAddress = &floodplainnC::__kdriverEntry;
		break;

	default:
		printf(FATAL"Proc 0x%x: Currently unsupported process type "
			"%d.\n",
			self->getFullId(), initBlock->type);

		panic(ERROR_UNSUPPORTED);
		break;
	};

	if (executableFormat != processStreamC::RAW)
	{
		// Load the confluence lib and extract its entry point addr.
		printf(FATAL"Proc 0x%x: Currently unsupported executable "
			"format %d.\n",
			self->getFullId(), executableFormat);

		callbackMessage->error = ERROR_UNSUPPORTED;
		err = messageStreamC::enqueueOnThread(
			self->parent->parentThreadId, callbackMessage);

		if (err != ERROR_SUCCESS) {
			panic(FATAL"commonEntry: Failed to queue callback\n");
		};

		// Until we have a destroyStream(), we just dormant it.
		taskTrib.dormant(self->getFullId());
	};

	/* For RAW processes, jumpAddress is set by their get*ExecutableFormat.
	 * The jumpAddress is always an address within the kernel image.
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
	registerContextC		context(self->parent->execDomain);

	err = context.initialize();
	if (err != ERROR_SUCCESS) { for (;;) { asm volatile("hlt\n\t"); }; };

	/* Kernel domain processes must use kernel space stack; user domain
	 * processes must use userspace stack.
	 **/
	context.setEntryPoint((void (*)(void *))jumpAddress);
	context.setStacks(
		self->stack0, self->stack1,
		(self->parent->execDomain == PROCESS_EXECDOMAIN_USER) ? 1 : 0);

	callbackMessage->error = ERROR_SUCCESS;
	err = messageStreamC::enqueueOnThread(
		self->parent->parentThreadId, callbackMessage);

	if (err != ERROR_SUCCESS)
	{
		panic(FATAL"commonEntry: Failed to queue callback.\n");
		// Until we have a destroyStream(), we just dormant it.
		taskTrib.dormant(self->getFullId());
	};

	// If the process was spawned with DORMANT set, dormant it now.
	if (__KFLAG_TEST(self->parent->flags, PROCESS_FLAGS_SPAWNED_DORMANT))
		{ taskTrib.dormant(self->getFullId()); };

	loadContextAndJump(&context);
}

processStreamC *processTribC::getStream(processId_t id)
{
	processStreamC	*ret;
	uarch_t		rwFlags;

	if (PROCID_PROCESS(id) == 0) {
		panic(FATAL PROCTRIB"getStream: Attempt to get process 0.\n");
	};

	processes.lock.readAcquire(&rwFlags);
	ret = processes.rsrc[PROCID_PROCESS(id)];
	processes.lock.readRelease(rwFlags);

	return ret;
}

error_t processTribC::getNewProcessId(processId_t *ret)
{
	uarch_t		rwFlags;
	sbit32		tmpId;

	processes.lock.readAcquire(&rwFlags);
	tmpId = nextProcId.getNextValue((void **)processes.rsrc);
	processes.lock.readRelease(rwFlags);

	if (tmpId < 0) { return ERROR_RESOURCE_EXHAUSTED; };
	if (tmpId <= PROCID_PROCESS(__KPROCESSID))
	{
		panic(FATAL PROCTRIB"getNewProcessId: attempt to hand out a "
			"reserved procid.\n");
	};

	*ret = (processId_t)tmpId << PROCID_PROCESS_SHIFT;
	return ERROR_SUCCESS;
}

static sarch_t deviceExists(utf8Char *devPath, fplainn::deviceC **dev)
{
	error_t			ret;

	ret = floodplainn.getDevice(devPath, dev);
	if (ret != ERROR_SUCCESS) { return 0; };
	return 1;
}

error_t processTribC::spawnDriver(
	utf8Char *commandLine,
	utf8Char *environment,
	taskC::schedPolicyE schedPolicy,
	ubit8 prio,
	uarch_t flags,
	void *privateData,
	driverProcessC **retProcess
	)
{
	error_t				ret;
	processId_t			newProcessId;
	threadC				*parentThread, *firstThread;
	fplainn::deviceC		*dev;
	fplainn::driverC		*drv;
	heapPtrC<driverProcessC>	newProcess;

	if (commandLine == NULL || retProcess == NULL)
		{ return ERROR_INVALID_ARG; };

	if (cpuTrib.getCurrentCpuStream()->taskStream.getCurrentTask()
		->getType() == task::PER_CPU)
	{
		panic(FATAL PROCTRIB"spawnDriver: called from per-cpu "
			"thread.\n");
	};

	parentThread = (threadC *)cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentTask();

	ret = getNewProcessId(&newProcessId);
	if (ret != ERROR_SUCCESS)
	{
		printf(NOTICE PROCTRIB"Out of process IDs.\n");
		return ret;
	};

	/**	FIXME:
	 * In reality, we are supposed to call on the driver search service to
	 * first determine if a driver can be found for this device, and if so,
	 * whether the driver needs to be spawned as a userspace or kernelspace
	 * process, and then allocate the correct type of PCB object.
	 *
	 * For now, we just always spawn a kernelspace process because we don't
	 * even support userspace driver processes as yet anyway.
	 **/
	// Verify that the device exists.
	if (!deviceExists(commandLine, &dev))
		{ return ERROR_RESOURCE_UNAVAILABLE; };

	newProcess = new driverProcessC(
		newProcessId, parentThread->getFullId(),
		(dev->isKernelDriver)
			? PROCESS_EXECDOMAIN_KERNEL
			: PROCESS_EXECDOMAIN_USER,
		dev->bankId,
		privateData);

	if (newProcess.get() == NULL)
	{
		printf(NOTICE PROCTRIB"Failed to alloc dtrib process.\n");
		return ERROR_MEMORY_NOMEM;
	};

	bitmapC		affinity;
	//numaCpuBankC	*ncb;

	// FIXME: Initialize the BMP to the device's affine bank.
	// Get the device's object
	// Get its bankId
	// ncb = cpuTrib.getBank(deviceBankId);
	// if (ncb == NULL) { fail or something; };
	// Then set the affinity below to merge with ncb->cpus.
	ret = affinity.initialize(cpuTrib.onlineCpus.getNBits());
	if (ret != ERROR_SUCCESS) { return ret; };
	affinity.merge(&cpuTrib.onlineCpus);

	ret = newProcess->initialize(commandLine, environment, &affinity);
	if (ret != ERROR_SUCCESS) { return ret; };

	// Verify that a driver has been detected for the device.
	if (!dev->driverDetected) { return SPAWNDRIVER_STATUS_NO_DRIVER; };
	// Verify that a driver has been loaded for the device.
	if (dev->driver == NULL)
	{
		// Force caller to call loadDriverReq() first.
		if (floodplainn.findDriver(dev->driverFullName, &drv)
			!= ERROR_SUCCESS)
		{
			printf(WARNING PROCTRIB"spawnDriver: %s: Must "
				"call loadDriver first.\n",
				commandLine);

			return ERROR_UNINITIALIZED;
		};

		dev->driver = drv;
	};

	// Add the new process to the process list.
	processes.setSlot(newProcessId, newProcess.get());

	// Spawn the first thread. Pass on the DORMANT flag if set.
	if (__KFLAG_TEST(flags, SPAWNPROC_FLAGS_DORMANT))
	{
		__KFLAG_SET(
			newProcess->flags,
			PROCESS_FLAGS_SPAWNED_DORMANT);
	};

	ret = newProcess->spawnThread(
		&processTribC::commonEntry, NULL,
		&newProcess->cpuAffinity,
		schedPolicy, prio,
		flags | SPAWNTHREAD_FLAGS_AFFINITY_SET
		| SPAWNTHREAD_FLAGS_FIRST_THREAD,
		&firstThread);

	if (ret != ERROR_SUCCESS)
	{
		printf(NOTICE"Failed to spawn thread for new driver.\n");
		processes.clearSlot(newProcessId);
		return ret;
	};

	printf(NOTICE"New driver spawned, tid = 0x%x.\n",
		firstThread->getFullId());

	*retProcess = newProcess.release();
	return ERROR_SUCCESS;
}

error_t processTribC::spawnDistributary(
	utf8Char *commandLine,
	utf8Char *environment,
	numaBankId_t addrSpaceBinding,
	ubit8 /*schedPrio*/,
	uarch_t /*flags*/,
	void *privateData,
	distributaryProcessC **newProcess
	)
{
	error_t			ret;
	processId_t		newProcessId;
	threadC			*parentThread, *firstTask;

	if (commandLine == NULL || newProcess == NULL)
		{ return ERROR_INVALID_ARG; };

	// Per-CPU threads are not allowed to spawn new processes.
	if (cpuTrib.getCurrentCpuStream()->taskStream.getCurrentTask()
		->getType() == task::PER_CPU)
	{
		panic(FATAL PROCTRIB"spawnDistributary: called from per-cpu "
			"thread.\n");
	};

	parentThread = (threadC *)cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentTask();

	ret = getNewProcessId(&newProcessId);
	if (ret != ERROR_SUCCESS)
	{
		printf(NOTICE PROCTRIB"Out of process IDs.\n");
		return ret;
	};

	*newProcess = new distributaryProcessC(
		newProcessId, parentThread->getFullId(),
		addrSpaceBinding, privateData);

	if (*newProcess == NULL)
	{
		printf(NOTICE PROCTRIB"Failed to alloc dtrib process.\n");
		return ERROR_MEMORY_NOMEM;
	};

	bitmapC		affinity;

	ret = affinity.initialize(cpuTrib.onlineCpus.getNBits());
	if (ret != ERROR_SUCCESS) { return ret; };
	affinity.merge(&cpuTrib.onlineCpus);

	ret = (*newProcess)->initialize(commandLine, environment, &affinity);
	if (ret != ERROR_SUCCESS) { return ret; };
	processes.setSlot(newProcessId, *newProcess);

	// Spawn the first thread.
	ret = (*newProcess)->spawnThread(
		&processTribC::commonEntry, NULL,
		&(*newProcess)->cpuAffinity,
		taskC::ROUND_ROBIN, 0,
		SPAWNTHREAD_FLAGS_AFFINITY_SET
		| SPAWNTHREAD_FLAGS_SCHEDPOLICY_SET
		| SPAWNTHREAD_FLAGS_FIRST_THREAD,
		&firstTask);

	if (ret != ERROR_SUCCESS)
	{
		printf(NOTICE"Failed to spawn thread for new process.\n");
		delete *newProcess;
		processes.clearSlot(newProcessId);
		return ret;
	};

	printf(NOTICE"New process spawned, tid = 0x%x.\n",
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
	void *privateData,
	error_t *err				// Returned error value.
	)
{
	processId_t		newId, parentId;
	sbit32			newIdTmp;
	uarch_t			rwFlags;
	processStreamC		*newProc=NULL;
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
		printf(WARNING PROCTRIB"Out of process IDs.\n");
		*err = ERROR_GENERAL;
		return NULL;
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
		return NULL;
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
		if (cpuAffinity != NULL)
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

			if (*err != ERROR_SUCCESS) { return NULL; };
		};
	};

	// TODO: Handle elevation.

	// 
	return newProc;
}
#endif

