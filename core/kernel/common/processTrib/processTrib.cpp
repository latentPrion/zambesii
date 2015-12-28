
#include <debug.h>
#include <chipset/memory.h>
#include <__kstdlib/__kcxxlib/memory>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kclasses/debugPipe.h>
#include <commonlibs/libzbzcore/libzbzcore.h>
#include <kernel/common/panic.h>
#include <kernel/common/messageStream.h>
#include <kernel/common/timerTrib/timerTrib.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/taskTrib/taskTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/vfsTrib/vfsTrib.h>
#include <kernel/common/floodplainn/floodplainn.h>
#include <__kthreads/main.h>


sPriorityClass	prioClasses[PRIOCLASS_NCLASSES];

void ProcessTrib::fillOutPrioClasses(void)
{
	new (&prioClasses[0]) sPriorityClass(CC"Idle", 0);
	new (&prioClasses[1]) sPriorityClass(CC"Low", 4);
	new (&prioClasses[2]) sPriorityClass(CC"Moderate", 8);
	new (&prioClasses[3]) sPriorityClass(CC"High", 12);
	new (&prioClasses[4]) sPriorityClass(CC"Critical", 16);
}

static inline error_t resizeAndMergeBitmaps(Bitmap *dest, Bitmap *src)
{
	error_t		ret;

	ret = dest->resizeTo(src->getNBits());
	if (ret != ERROR_SUCCESS) { return ret; };

	dest->merge(src);
	return ERROR_SUCCESS;
}

error_t ProcessTrib::getDistributaryExecutableFormat(
	utf8Char *fullName, ProcessStream::executableFormatE *executableFormat,
	void (**entryPoint)(void)
	)
{
	dvfs::Tag			*tag;
	error_t				ret;
	Thread				*self;

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
	*executableFormat = ProcessStream::RAW;
	self = (Thread *)cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentThread();

	ret = vfsTrib.getDvfs()->getPath(fullName, &tag);

	if (ret != ERROR_SUCCESS)
	{
		printf(ERROR PROCTRIB"Proc 0x%x: command line invalid; "
			"distributary doesn't exist.\n",
			self->getFullId());

		return SPAWNPROC_STATUS_INVALID_FILE_NAME;
	};

	if (tag->getDInode()->getType() == dvfs::DistributaryInode::IN_KERNEL)
	{
		// IN_KERNEL dtribs are raw embedded code in the kernel image.
		// *entryPoint = tag->getDInode()->getEntryAddress();
		*entryPoint = &__klzbzcore::main;
		return ERROR_SUCCESS;
	}
	else
	{
		// OUT_OF_KERNEL distributaries can only be ELF.
		printf(WARNING PROCTRIB"Proc 0x%x: out-of-kernel dtribs are "
			"not yet supported.\n",
			self->getFullId());

		return getExecutableFormat(fullName, executableFormat);
	};
}

error_t ProcessTrib::getDriverExecutableFormat(
	utf8Char *fullName, ProcessStream::executableFormatE *retfmt,
	void (**retEntryPoint)(void)
	)
{
	error_t			ret;
	fplainn::Driver	*tmpDrv;

	ret = floodplainn.zudi.findDriver(fullName, &tmpDrv);
	if (ret != ERROR_SUCCESS) { return ERROR_UNINITIALIZED; };

	if (tmpDrv->index == fplainn::Zui::INDEX_KERNEL)
	{
		*retfmt = ProcessStream::RAW;
		*retEntryPoint = &__klzbzcore::main;
		return ERROR_SUCCESS;
	};

	return getExecutableFormat(fullName, retfmt);
}

error_t ProcessTrib::getExecutableFormat(
	utf8Char *, ProcessStream::executableFormatE *
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

static ubit32 getCallbackFunctionNo(Thread *self)
{
	switch (self->parent->getType())
	{
	case ProcessStream::DISTRIBUTARY:
		return MSGSTREAM_PROCESS_SPAWN_DISTRIBUTARY;
	case ProcessStream::DRIVER:
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
void ProcessTrib::commonEntry(void *)
{
	Thread						*self;
	ProcessStream::sInitializationBlockizeInfo	initBlockSizes;
	ProcessStream::sInitializationBlock		*initBlock;
	ProcessStream::executableFormatE		executableFormat;
	MessageStream::sHeader				*callbackMessage;
	uarch_t						initBlockNPages;
	error_t						err;
	void						(*jumpAddress)(void);
	AsyncResponse					myResponse;

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
	self = (Thread *)cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentThread();

	printf(NOTICE PROCTRIB"New process running. ID=0x%x type=%d.\n",
		self->getFullId(),
		self->parent->getType());

	// Allocate the callback message memory.
	callbackMessage = new MessageStream::sHeader(
		self->parent->parentThreadId,
		MSGSTREAM_SUBSYSTEM_PROCESS, getCallbackFunctionNo(self),
		sizeof(*callbackMessage), 0, self->parent->privateData);

	if (callbackMessage == NULL)
	{
		printf(FATAL PROCTRIB"commonEntry: process 0x%x:\n",
			self->getFullId());

		panic(CC"\tFailed to allocate callback message.\n");
	};

	myResponse(callbackMessage);

	// Allocate initialization block memory.
	self->parent->getInitializationBlockSizeInfo(&initBlockSizes);
	initBlockNPages = PAGING_BYTES_TO_PAGES(
		sizeof(ProcessStream::sInitializationBlock)
			+ initBlockSizes.fullNameSize + 1
			+ initBlockSizes.workingDirectorySize + 1
			+ initBlockSizes.argumentsSize + 1);

	initBlock = new (
		self->parent->memoryStream.memAlloc(initBlockNPages))
		ProcessStream::sInitializationBlock;

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
	case (ubit8)ProcessStream::DISTRIBUTARY:
		err = getDistributaryExecutableFormat(
			initBlock->fullName, &executableFormat, &jumpAddress);

		break;

	case (ubit8)ProcessStream::DRIVER:
		err = getDriverExecutableFormat(
			initBlock->fullName, &executableFormat, &jumpAddress);

		break;

	default:
		err = ERROR_UNSUPPORTED;
		printf(FATAL PROCTRIB"Proc 0x%x: Currently unsupported process "
			"type %d.\n",
			self->getFullId(), initBlock->type);

		myResponse(err);
		// Prematurely destroy the object to force-send the message.
		myResponse.~AsyncResponse();
		taskTrib.kill(self->getFullId());

		__builtin_unreachable();
	};

	if (err != ERROR_SUCCESS)
	{
		printf(ERROR PROCTRIB"Proc 0x%x: Failed to detect process' "
			"executable format.\n",
			self->getFullId());

		myResponse(ERROR_UNSUPPORTED);
		myResponse.~AsyncResponse();
		taskTrib.dormant(self->getFullId());
	};

	if (executableFormat != ProcessStream::RAW)
	{
		// Load the confluence lib and extract its entry point addr.
		printf(FATAL PROCTRIB"Proc 0x%x: Currently unsupported "
			"executable format %d.\n",
			self->getFullId(), executableFormat);

		myResponse(ERROR_UNSUPPORTED);
		myResponse.~AsyncResponse();
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
	RegisterContext		context(self->parent->execDomain);

	err = context.initialize();
	if (err != ERROR_SUCCESS)
	{
		printf(FATAL PROCTRIB"Proc 0x%x: Failed to initialize reg "
			"context; unable to jump to rawsyslib entry.\n",
			self->getFullId());

		myResponse(err);
		myResponse.~AsyncResponse();
		taskTrib.dormant(self->getFullId());
	};

	/* Kernel domain processes must use kernel space stack; user domain
	 * processes must use userspace stack.
	 **/
	context.setEntryPoint((void (*)(void *))jumpAddress);
	context.setStacks(
		self->stack0, self->stack1,
		(self->parent->execDomain == PROCESS_EXECDOMAIN_USER) ? 1 : 0);

	/* Now we set the response message to the variable in the process's PCB.
	 * We do not yet know whether or not the process will be loaded
	 * correctly after we hand off to the syslib. It could well happen that,
	 * for example, libzbzcore may fail to find one of the libraries
	 * required by this new process.
	 *
	 * So we just store the response message away, and when the syslib is
	 * sure that it has loaded the process correctly, it will syscall into
	 * the kernel and tell the kernel to send the response message to the
	 * parent process.
	 **/
	self->parent->responseMessage = callbackMessage;
	myResponse(DONT_SEND_RESPONSE);

	// If the process was spawned with DORMANT set, dormant it now.
	if (FLAG_TEST(self->parent->flags, PROCESS_FLAGS_SPAWNED_DORMANT))
		{ taskTrib.dormant(self->getFullId()); };

	loadContextAndJump(&context);
}

ProcessStream *ProcessTrib::getStream(processId_t id)
{
	ProcessStream	*ret;
	uarch_t		rwFlags;

	if (PROCID_PROCESS(id) == CPU_PROCESSID) {
		return __kgetStream();
	};

	processes.lock.readAcquire(&rwFlags);
	ret = processes.rsrc[PROCID_PROCESS(id)];
	processes.lock.readRelease(rwFlags);

	return ret;
}

error_t ProcessTrib::getNewProcessId(processId_t *ret)
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

static sarch_t deviceExists(utf8Char *devPath, fplainn::Device **dev)
{
	error_t			ret;

	ret = floodplainn.getDevice(devPath, dev);
	if (ret != ERROR_SUCCESS) { return 0; };
	return 1;
}

error_t ProcessTrib::spawnDriver(
	utf8Char *commandLine,
	utf8Char *environment,
	ubit8 prio,
	uarch_t flags,
	void *privateData,
	DriverProcess **retProcess
	)
{
	error_t				ret;
	processId_t			newProcessId;
	Thread				*parentThread, *firstThread;
	fplainn::Device			*dev;
	fplainn::Driver			*drv;
	fplainn::DriverInstance		*drvInstance;
	HeapObj<DriverProcess>		newProcess;

	if (commandLine == NULL || retProcess == NULL)
		{ return ERROR_INVALID_ARG; };

	parentThread = (Thread *)cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentThread();

	ret = getNewProcessId(&newProcessId);
	if (ret != ERROR_SUCCESS)
	{
		printf(NOTICE PROCTRIB"Out of process IDs.\n");
		return ret;
	};

	/**	FIXME:
	 * Problem here is that "commandLine" could contain components other
	 * than the device's VFS path, such as...actual command line arguments.
	 * Solution would be to isolate the command line parsing code out of
	 * the process object, so we can call it in other code modules as well.
	 **/
	// Verify that the device exists.
	if (!deviceExists(commandLine, &dev))
		{ return ERROR_RESOURCE_UNAVAILABLE; };

	// Verify that a driver has been detected for the device.
	if (!dev->driverDetected) { return SPAWNDRIVER_STATUS_NO_DRIVER; };
	// Verify that caller has called loadDriverReq() first.
	if (floodplainn.zudi.findDriver(dev->driverFullName, &drv) != ERROR_SUCCESS)
	{
		printf(WARNING PROCTRIB"spawnDriver: %s: Must successfully "
			"call loadDriver first.\n",
			commandLine);

		return ERROR_UNINITIALIZED;
	};

	/* Look through the instances attached to the driver for a
	 * driver instance that matches the NUMA domain of the device. If we
	 * find one, it means that the driver process has already been spawned.
	 **/
	drvInstance = drv->getInstance(dev->bankId);
	if (drvInstance != NULL)
	{
		*retProcess = (DriverProcess *)processTrib.getStream(
			drvInstance->pid);

		return ERROR_SUCCESS;
	};

	/* Else, we should proceed to spawn a process for the driver. Start by
	 * adding a new driver instance to driver's instance list.
	 **/
	ret = drv->addInstance(dev->bankId, newProcessId);
	if (ret != ERROR_SUCCESS) { return ret; };

	drvInstance = drv->getInstance(dev->bankId);

	newProcess = new DriverProcess(
		newProcessId, parentThread->getFullId(),
		(drv->index == fplainn::Zui::INDEX_KERNEL)
			? PROCESS_EXECDOMAIN_KERNEL
			: PROCESS_EXECDOMAIN_USER,
		dev->bankId, drvInstance,
		privateData);

	if (newProcess.get() == NULL)
	{
		printf(NOTICE PROCTRIB"Failed to alloc driver process.\n");
		return ERROR_MEMORY_NOMEM;
	};

	Bitmap			affinity;
	//NumaCpuBank		*ncb;

	// FIXME: Initialize the BMP to the device's affine bank.
	// Get the device's object
	// Get its bankId
	// ncb = cpuTrib.getBank(deviceBankId);
	// if (ncb == NULL) { fail or something; };
	// Then set the affinity below to merge with ncb->cpus.
	ret = affinity.initialize(cpuTrib.onlineCpus.getNBits());
	if (ret != ERROR_SUCCESS) { return ret; };
	affinity.merge(&cpuTrib.onlineCpus);

	ret = newProcess->initialize(dev->driverFullName, environment, &affinity);
	if (ret != ERROR_SUCCESS) { return ret; };

	// Add the new process to the process list.
	processes.setSlot(newProcessId, newProcess.get());

	// Set driver process instance for the device.
	dev->driverInstance = drv->getInstance(dev->bankId);

	// Spawn the first thread. Pass on the DORMANT flag if set.
	if (FLAG_TEST(flags, SPAWNPROC_FLAGS_DORMANT))
		{ FLAG_SET(newProcess->flags, PROCESS_FLAGS_SPAWNED_DORMANT); };

	// All drivers are inherently realtime.
	ret = newProcess->spawnThread(
		&ProcessTrib::commonEntry, NULL,
		&newProcess->cpuAffinity,
		Thread::REAL_TIME, prio,
		flags | SPAWNTHREAD_FLAGS_AFFINITY_SET
		| SPAWNTHREAD_FLAGS_FIRST_THREAD,
		&firstThread);

	if (ret != ERROR_SUCCESS)
	{
		printf(NOTICE PROCTRIB"Failed to spawn thread for new driver.\n");
		processes.clearSlot(newProcessId);
		return ret;
	};

	printf(NOTICE PROCTRIB"spawnDriver: New driver spawned, tid = 0x%x.\n",
		firstThread->getFullId());

	*retProcess = newProcess.release();
	return ERROR_SUCCESS;
}

error_t ProcessTrib::spawnDistributary(
	utf8Char *commandLine,
	utf8Char *environment,
	numaBankId_t addrSpaceBinding,
	ubit8 /*schedPrio*/,
	uarch_t /*flags*/,
	void *privateData,
	DistributaryProcess **newProcess
	)
{
	error_t			ret;
	processId_t		newProcessId;
	Thread			*parentThread, *firstTask;

	if (commandLine == NULL || newProcess == NULL)
		{ return ERROR_INVALID_ARG; };

	parentThread = (Thread *)cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentThread();

	ret = getNewProcessId(&newProcessId);
	if (ret != ERROR_SUCCESS)
	{
		printf(NOTICE PROCTRIB"Out of process IDs.\n");
		return ret;
	};

	*newProcess = new DistributaryProcess(
		newProcessId, parentThread->getFullId(),
		addrSpaceBinding, privateData);

	if (*newProcess == NULL)
	{
		printf(NOTICE PROCTRIB"Failed to alloc dtrib process.\n");
		return ERROR_MEMORY_NOMEM;
	};

	Bitmap		affinity;
	ret = affinity.initialize(cpuTrib.onlineCpus.getNBits());
	if (ret != ERROR_SUCCESS) { return ret; };
	affinity.merge(&cpuTrib.onlineCpus);

	ret = (*newProcess)->initialize(commandLine, environment, &affinity);
	if (ret != ERROR_SUCCESS) { return ret; };
	processes.setSlot(newProcessId, *newProcess);

	// Spawn the first thread.
	ret = (*newProcess)->spawnThread(
		&ProcessTrib::commonEntry, NULL,
		&(*newProcess)->cpuAffinity,
		Thread::ROUND_ROBIN, 0,
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
error_t *ProcessTrib::spawnStream(
	numaBankId_t,				// NUMA addrspace binding.
	Bitmap *cpuAffinity,			// Oceann/NUMA/SMP affinity.
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
	ProcessStream		*newProc=NULL;
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
	if (FLAG_TEST(flags, PROCESSTRIB_SPAWN_FLAGS_PINHERIT_AFFINITY))
	{
		// Inherit affinity from spawning process.
		*err = resizeAndMergeBitmaps(
			&newProc->cpuAffinity,
			&cpuTrib.getCurrentCpuStream()
				->taskStream.getCurrentTask()->parent->cpuAffinity);
	}
	else if (FLAG_TEST(flags, PROCESSTRIB_SPAWN_FLAGS_STINHERIT_AFFINITY))
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

void ProcessTrib::dumpProcessesAndVaddrspaces(void)
{
	uarch_t			flipFlop=0;

	printf(NOTICE PROCTRIB"Dumping all PIDs and vaddrspaces.\n");
	for (uarch_t i=0; i<CHIPSET_MEMORY_MAX_NPROCESSES; i++)
	{
		VaddrSpaceStream		*v;

		if (i == 0 || processes.rsrc[i] == NULL) { continue; };

		if (flipFlop == 0) { printf(CC"\t"); };

		v = processes.rsrc[i]->getVaddrSpaceStream();
		printf(CC"PID 0x%x l0v 0x%p l0p 0x%P; ",
			processes.rsrc[i]->id,
			v->vaddrSpace.level0Accessor.rsrc,
			&v->vaddrSpace.level0Paddr);

		flipFlop++;

		if (flipFlop == 1)
		{
			printf(CC"\n");
			flipFlop = 0;
		};
	};

	if (flipFlop != 0) { printf(CC"\n"); };
}
