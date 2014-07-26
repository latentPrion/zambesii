
#include <debug.h>
#include <arch/atomic.h>
#include <chipset/chipset.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kstdlib/__kcxxlib/new>
#include <kernel/common/panic.h>
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


#if __SCALING__ >= SCALING_SMP
// Preallocated mem space for the internals of the __kprocess Bitmap objects.
ubit8		__kprocessPreallocatedBmpMem[3][32];
#endif

error_t processStreamC::initialize(
	const utf8Char *commandLineString, const utf8Char *environmentString,
	Bitmap *cpuAffinityBmp
	)
{
	error_t		ret;
	ubit16		argumentsStartIndex;

	ret = nextTaskId.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

	ret = generateFullName(commandLineString, &argumentsStartIndex);
	if (ret != ERROR_SUCCESS) { return ret; };
	// N-n-next split n-n-next split...
	ret = generateArguments(&commandLineString[argumentsStartIndex]);
	if (ret != ERROR_SUCCESS) { return ret; };
	if (environmentString != NULL)
	{
		ret = generateEnvironment(environmentString);
		if (ret != ERROR_SUCCESS) { return ret; };
	};

	// Initialize internal bitmaps.
	ret = initializeBitmaps();
	if (ret != ERROR_SUCCESS) { return ret; };

	if (cpuAffinityBmp != NULL)
	{
		Bitmap		cpuAffinityTmp;

		ret = cpuAffinityTmp.initialize(cpuAffinityBmp->getNBits());
		if (ret != ERROR_SUCCESS) { return ret; };

		cpuAffinityTmp.merge(cpuAffinityBmp);

		/* If the BMP passed by the caller has more bits than there are
		 * CPUs, we truncate its bits.
		 **/
		if (cpuAffinityTmp.getNBits()
			> cpuTrib.availableCpus.getNBits())
		{
			ret = cpuAffinityTmp.resizeTo(
				cpuTrib.availableCpus.getNBits());

			if (ret != ERROR_SUCCESS) { return ret; };
		};

		// Finally, set the new process' affinity.
		cpuAffinity.merge(&cpuAffinityTmp);
	}
	else {
		// If no affinity given, assume default = "all online cpus".
		cpuAffinity.merge(&cpuTrib.onlineCpus);
	};

	/* Now initialize all the process' streams if it's not the kernel
	 * process. The kernel process' streams are initialized separately
	 * as a result of the kernel initialization sequence.
	 **/
	if (this->id != __KPROCESSID)
	{
		ret = memoryStream.initialize();
		if (ret != ERROR_SUCCESS) { return ret; };
		ret = timerStream.initialize();
		if (ret != ERROR_SUCCESS) { return ret; };
		ret = floodplainnStream.initialize();
		if (ret != ERROR_SUCCESS) { return ret; };
		ret = zasyncStream.initialize();
		if (ret != ERROR_SUCCESS) { return ret; };
	};

	return ERROR_SUCCESS;
}

error_t processStreamC::initializeBitmaps(void)
{
	error_t		ret;

	if (id == __KPROCESSID)
	{
#if __SCALING__ >= SCALING_SMP
		cpuTrace.initialize(
			0,
			Bitmap::preallocatedMemoryS(
				__kprocessPreallocatedBmpMem[0],
				sizeof(__kprocessPreallocatedBmpMem[0])));

		cpuAffinity.initialize(
			0,
			Bitmap::preallocatedMemoryS(
				__kprocessPreallocatedBmpMem[1],
				sizeof(__kprocessPreallocatedBmpMem[1])));
#endif
		pendingEvents.initialize(
			0,
			Bitmap::preallocatedMemoryS(
				__kprocessPreallocatedBmpMem[2],
				sizeof(__kprocessPreallocatedBmpMem[2])));
	}
	else
	{
#if __SCALING__ >= SCALING_SMP
		ret = cpuTrace.initialize(cpuTrib.availableCpus.getNBits());
		if (ret != ERROR_SUCCESS) { return ret; };
		ret = cpuAffinity.initialize(cpuTrib.availableCpus.getNBits());
		if (ret != ERROR_SUCCESS) { return ret; };
#endif
		pendingEvents.initialize(32);
		if (ret != ERROR_SUCCESS) { return ret; };
	};

	return ERROR_SUCCESS;
}

processStreamC::~processStreamC(void)
{
}

/* While the maxlength of fullName is 4000, the kernel process' full name will
 * never actually be that long anyway.
 **/
static utf8Char		__kprocessFullNameMem[256+1];
error_t processStreamC::generateFullName(
	const utf8Char *_commandLine, ubit16 *argumentsStartIndex
	)
{
	error_t		ret;
	// 0=un-escaped, 1=single-quote, 2=double-quote.
	ubit8		escapeType=0;
	ubit16		length=0, nEscapedSpaces=0;

	/**	EXPLANATION:
	 * "_commandLine" passed to spawnStream() shall be a pair of strings
	 * separated by ASCII space, concatenated into one, of maximum length:
	 *	PROCESS_FULLNAME_MAXLEN + 1 + PROCESS_ARGUMENTS_MAXLEN + 1.
	 *
	 * The extra character (+1 in the middle) is provided to allow the
	 * separating ASCII space character between the two concatenated
	 * strings; or, if the second string is not provided, the extra char in
	 * the middle should be used to NULL terminate the first string, if
	 * necessary. The first string SHALL either be SPACE or NULL terminated,
	 * whether or not it occupies PROCESS_FULLNAME_MAXLEN characters. The
	 * second string SHALL be NULL terminated as well, whether or not it
	 * takes occupies PROCESS_FULLNAME_MAXLEN characters.
	 *
	 * The first string is the process executable image's FULL path+filename
	 * and is mandatory. The second string is a series of arguments passed
	 * to the new process, which goes uninterpreted by the kernel
	 * altogether, and is passed raw to the new process when it requests it.
	 *
	 * The filename is terminated at the first un-escaped space (spaces can
	 * be escaped with ASCII backslash, or else the whole filename can be
	 * quoted) or the first NULL character encountered. The quotes count
	 * toward the full filename limit. The first character in the filename
	 * cannot be a space.
	 *
	 * If the kernel first encounters an unescaped space, it will finish
	 * parsing for the filename, and begin parsing for the arguments
	 * immediately following the space.
	 *
	 * If the kernel first encounters NULL (escaped or not) it will finish
	 * parsing the string altogether and assume that there were no
	 * arguments following the filename.
	 *
	 * Use the syscall processTrib.getProcessFullnameMaxLength() and
	 * processTrib.getProcessArgumentsMaxLength() if you think your app
	 * will generate an executable name or arguments string longer than
	 * the system limits.
	 *
	 * Examples:
	 *	/path/file arg arg arg
	 *	=> name="/path/file", args="arg arg arg"
	 *	/path/with spaces/file arg arg arg
	 *	=> name="/path/with", args="spaces/file arg arg arg"
	 *	'/quoted/path/with spaces/file' arg arg arg
	 *	=> name="/quoted/path/with spaces/file", args="arg arg arg"
	 *	"/quoted/path/with spaces/file" arg arg arg
	 *	=> name="/quoted/path/with spaces/file", args="arg arg arg"
	 *	/path/file
	 *	=> name="/path/file", args=""
	 **/
	if (_commandLine[0] == '\'') { escapeType = 1; _commandLine++; };
	if (_commandLine[0] == '"') { escapeType = 2; _commandLine++; };

	if (escapeType > 0)
	{
		/* If the image fullname is escaped with quotes, find the ending
		 * quote, and assume that will give us the full length of the
		 * string.
		 *
		 *	FIXME:
		 * The current parser does not enable escaping of quotes in
		 * the filename. It will absolutely take the next quote to be
		 * the ending quote. Add support for escaping. This also
		 * mandates that backslash escapes be supported too (\\).
		 **/
		ret = vfs::getIndexOfNext(
			CC _commandLine, (escapeType == 1) ? '\'' : '"',
			PROCESS_FULLNAME_MAXLEN-1);

		if (ret < 0) { return ret; };
		length = (unsigned)ret;

		/* Check for sanity after the quote; must have a space or NULL
		 * after it.
		 **/
		if (_commandLine[length+1] != ' '
			&& _commandLine[length+1] != '\0')
		{
			return ERROR_INVALID_RESOURCE_NAME;
		};
	}
	else
	{
		/* Else, run through looking for the first space character
		 * that is not preceded by a backslash (\).
		 **/
		ubit16		i;

		for (i=0; _commandLine[i] != '\0' && i<PROCESS_FULLNAME_MAXLEN;
			i++)
		{
			if (_commandLine[i] == ' ')
			{
				// Finding a space at index 0 will raise error.
				if (i == 0) { break; };
				if (_commandLine[i-1] == '\\')
				{
					nEscapedSpaces++;
					continue;
				} else { break; };
			};
		};

		if (i == PROCESS_FULLNAME_MAXLEN
			&& (_commandLine[i] != ' ' && _commandLine[i] != '\0'))
		{
			// Space or NULL must follow the filename.
			return ERROR_INVALID_RESOURCE_NAME;
		};

		length = i;
	};

	// If the process image length is 0, exit with an error.
	if (length == 0) { return ERROR_INVALID_RESOURCE_NAME; };

	// Allocate space for the fullName and copy the data.
	if (id == __KPROCESSID) {
		fullName = __kprocessFullNameMem;
	}
	else
	{
		fullName = new utf8Char[length + 1];
		if (fullName == NULL) { return ERROR_MEMORY_NOMEM; };
	};

	/* Can't just strncpy() here; have to consume the backslashes in the
	 * fullname while copying it.
	 **/
	for (ubit16 i=0, j=0, k=0; i<length && _commandLine[i] != '\0'; i++)
	{
		if (_commandLine[i] == ' ' && _commandLine[i-1] == '\\'
			&& k < nEscapedSpaces)
		{
			k++;
			if (j > 0) { j--; };
		};

		fullName[j++] = _commandLine[i];
	};

	fullName[length - nEscapedSpaces] = '\0';

	/* If the fullName ends in NULL and not space, make sure that arguments
	 * generation code will meet NULL as soon as it starts trying to
	 * parse the string.
	 **/
	if (_commandLine[length] == '\0') { *argumentsStartIndex = length; }
	else if (_commandLine[length] == ' ') {
		*argumentsStartIndex = length + 1;
	}
	else
	{
		// Else it's a quote.
		if (_commandLine[length+1] == ' ') {
			*argumentsStartIndex = length + 2 + 1;
		} else {
			*argumentsStartIndex = length + 1 + 1;
		};
	};

	return ERROR_SUCCESS;
}

static utf8Char		__kprocessArgumentsMem[2048+1];
error_t processStreamC::generateArguments(const utf8Char *argumentString)
{
	ubit16		length;
	const ubit16	maxLength = (id == __KPROCESSID)
		? 2048 : PROCESS_ARGUMENTS_MAXLEN;

	/* There is nothing to really parse here -- we don't care what the
	 * content of the process' argument string is. We just need to know how
	 * long it is so we can allocate memory to hold it in the kernel
	 * address space. We parse up to a maximum of PROCESS_ARGUMENTS_MAXLEN
	 * chars, so if we don't meet a NULL, we assume that the argument
	 * string is that long.
	 **/
	length = strnlen8(argumentString, maxLength);

	// Don't allow limit overflows.
	if (length == maxLength && argumentString[length] != '\0')
		{ return ERROR_LIMIT_OVERFLOWED; };

	if (id == __KPROCESSID) {
		arguments = __kprocessArgumentsMem;
	}
	else
	{
		arguments = new utf8Char[length + 1];
		if (arguments == NULL) { return ERROR_MEMORY_NOMEM; };
	};

	strncpy8(arguments, argumentString, length);
	arguments[length] = '\0';
	return ERROR_SUCCESS;
}

static processStreamC::environmentVarS		__kprocessEnvironmentMem[16];
error_t processStreamC::generateEnvironment(const utf8Char *environmentString)
{
	error_t			ret;
	ubit16			strIndex, nameLen, valueLen;
	ubit8			nVars=0;
	sarch_t			isSecondPass;
	ubit8			maxNVars;

	/**	EXPLANATION:
	 * Environment variable string is a series of NULL terminated strings
	 * concatenated. The whole sequence of strings is terminated with a
	 * double NULL sequence (NULL NULL).
	 *
	 * Format is:
	 *	"name=value\0name=value\0name=value\0\0"
	 *
	 * Names of environment variables can be anything, including unicode.
	 * There is no invalid character other than ASCII "equal" sign (=).
	 * Environment variable names can contain any character the caller
	 * chooses, other than '=' and '\0'. Variable names cannot be NULL.
	 *
	 * Values can be NULL (i.e: "name=\0" or "name\0"). ASCII "equal" sign
	 * is a valid character in a value. The only character not permitted in
	 * a value is ASCII NULL.
	 *
	 * If the first character in an environment string passed as an argument
	 * to this function is NULL, the kernel will assume that the string is
	 * a NULL string (and refuse to parse it).
	 *
	 * The parsing happens in two stages, first pass where we just count up
	 * the number of variables and allocate the total amount of memory
	 * required, and second pass, where we re-parse and copy the variables
	 * into the newly allocated memory.
	 **/
	maxNVars = (id == __KPROCESSID) ? 16 : PROCESS_ENV_MAX_NVARS;

	for (isSecondPass=0; isSecondPass < 2; isSecondPass++)
	{
		strIndex = 0;
		for (ubit8 i=0; i<maxNVars;
			i++, strIndex += nameLen + valueLen + 1)
		{
			if (environmentString[strIndex] == '\0') { break; };

			ret = vfs::getIndexOfNext(
				CC &environmentString[strIndex], '=',
				PROCESS_ENV_NAME_MAXLEN+1);

			// No maxlen overflows or zero-length names allowed.
			if (ret == ERROR_INVALID_RESOURCE_NAME || ret == 0)
				{ return ERROR_INVALID_FORMAT; };

			/* 2 cases left:
			 *	1. ret is ERROR_NOT_FOUND = variable with no
			 *	   value.
			 *	2. Normal case, found an equal sign.
			 *
			 * Both are valid.
			 **/
			valueLen = 0;
			nameLen = (ret == ERROR_NOT_FOUND)
				? strlen8(&environmentString[strIndex])
				: (unsigned)ret;

			nVars++;

			if (isSecondPass)
			{
				strncpy8(
					environment[i].name,
					&environmentString[strIndex], nameLen);

				if (nameLen < PROCESS_ENV_NAME_MAXLEN) {
					environment[i].name[nameLen] = '\0';
				};
			};

			if (ret == ERROR_NOT_FOUND)
			{
				if (isSecondPass)
					{ environment[i].value[0] = '\0'; };

				continue;
			};

			// Get the length of the value.
			valueLen = strnlen8(
				&environmentString[strIndex + nameLen + 1],
				PROCESS_ENV_VALUE_MAXLEN + 1);

			if (valueLen == PROCESS_ENV_VALUE_MAXLEN+1)
				{ return ERROR_LIMIT_OVERFLOWED; };

			if (isSecondPass)
			{
				strncpy8(
					environment[i].value,
					&environmentString[
						strIndex + nameLen + 1],
					valueLen);

				if (valueLen < PROCESS_ENV_VALUE_MAXLEN) {
					environment[i].value[valueLen] = '\0';
				};
			};

			// Consume one for the equal sign.
			strIndex++;
		};

		// Don't allocate twice.
		if (isSecondPass) { break; };

		if (id == __KPROCESSID) {
			environment = __kprocessEnvironmentMem;
		}
		else
		{
			environment = new environmentVarS[nVars];
			if (environment == NULL)
				{ return ERROR_MEMORY_NOMEM; };
		};

		nEnvVars = nVars;
		maxNVars = nVars;
	};

	return ERROR_SUCCESS;
}

void processStreamC::sendResponse(error_t err)
{
	error_t				tmpErr;
	messageStreamC::sHeader		*msg;

	// This syscall should only work once throughout the process' lifetime.
	if (this->responseMessage == NULL) { return; };

	msg = this->responseMessage;
	this->responseMessage = NULL;

	msg->error = err;
	tmpErr = messageStreamC::enqueueOnThread(msg->targetId, msg);
	if (tmpErr != ERROR_SUCCESS)
	{
		printf(FATAL"processStreamC::sendResponse(%d): proc 0x%x: "
			"Failed because %d.\n",
			err, this->id, tmpErr);
	};
}

void processStreamC::getInitializationBlockSizeInfo(
	initializationBlockSizeInfoS *ret
	)
{
	if (ret == NULL) { return; };

	ret->fullNameSize = strnlen8(fullName, PROCESS_FULLNAME_MAXLEN) + 1;
	ret->workingDirectorySize = 1;
	ret->argumentsSize = strnlen8(arguments, PROCESS_ARGUMENTS_MAXLEN) + 1;
}

void processStreamC::getInitializationBlock(initializationBlockS *ret)
{
	if (ret == NULL) { return; };

	if (ret->fullName != NULL) { strcpy8(ret->fullName, fullName); };

	if (ret->workingDirectory != NULL) {
		//strcpy8(ret->workingDirectory, workingDirectory);
		ret->workingDirectory[0] = '\0';
	};

	if (ret->arguments != NULL) {
		strcpy8(ret->arguments, arguments);
	};

	ret->type = (ubit8)getType();
	ret->execDomain = execDomain;
}

static inline error_t resizeAndMergeBitmaps(Bitmap *dest, Bitmap *src)
{
	error_t		ret;

	ret = dest->resizeTo(src->getNBits());
	if (ret != ERROR_SUCCESS) { return ret; };

	dest->merge(src);
	return ERROR_SUCCESS;
}

error_t processStreamC::spawnThread(
	void (*entryPoint)(void *), void *argument,
	Bitmap * cpuAffinity,
	Task::schedPolicyE schedPolicy,
	ubit8 prio, uarch_t flags,
	Thread **const newThread
	)
{
	error_t		ret;
	processId_t	newThreadId;

	if (newThread == NULL) { return ERROR_INVALID_ARG; };
	// For now, per-cpu threads are not allowed to spawn new threads.
	if (cpuTrib.getCurrentCpuStream()->taskStream.getCurrentTask()
		->getType() == task::PER_CPU)
	{
		panic(FATAL"spawnThread: Called by a per-cpu thread.\n");
	};

	// Check for a free thread ID in this process's thread ID space.
	ret = getNewThreadId(&newThreadId);
	if (ret != ERROR_SUCCESS) { return SPAWNTHREAD_STATUS_NO_PIDS; };

	// Combine the parent process ID with new thread ID for full thread ID.
	newThreadId = id | newThreadId;

	// Allocate new thread if ID was available.
	*newThread = allocateNewThread(newThreadId, argument);
	if (*newThread == NULL) { return ERROR_MEMORY_NOMEM; };

	// Allocate internal sub-structures (context, etc.).
	ret = (*newThread)->initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

	ret = (*newThread)->allocateStacks();
	if (ret != ERROR_SUCCESS) { return ret; };

#if __SCALING__ >= SCALING_SMP
	// Do affinity inheritance.
	ret = (*newThread)->getTaskContext()->inheritAffinity(
		cpuAffinity, flags);

	if (ret != ERROR_SUCCESS) { return ret; };
#endif

	(*newThread)->inheritSchedPolicy(schedPolicy, flags);
	(*newThread)->inheritSchedPrio(prio, flags);

	// Now everything is allocated; just initialize the register context.
	(*newThread)->getTaskContext()->initializeRegisterContext(
		entryPoint,
		(*newThread)->stack0,
		(*newThread)->stack1,
		this->execDomain,
		FLAG_TEST(flags, SPAWNTHREAD_FLAGS_FIRST_THREAD));

	/* First thread in a process is always scheduled immediately; DORMANT
	 * is handled by the kernel's common entry point for such threads. For
	 * other threads (which do not pass through the common entry point),
	 * DORMANT is handled right here.
	 **/
	if (!FLAG_TEST(flags, SPAWNTHREAD_FLAGS_DORMANT)
		|| FLAG_TEST(flags, SPAWNTHREAD_FLAGS_FIRST_THREAD))
	{
		return taskTrib.schedule(*newThread);
	} else {
		return ERROR_SUCCESS;
	};
}

Thread *processStreamC::allocateNewThread(processId_t newThreadId, void *privateData)
{
	Thread		*ret;

	ret = new Thread(newThreadId, this, privateData);
	if (ret == NULL) { return NULL; };

	taskLock.writeAcquire();

	// Add the new thread to the process's task array.
	tasks[PROCID_THREAD(newThreadId)] = ret;
	nTasks++;

	taskLock.writeRelease();
	return ret;
}

void processStreamC::removeThread(processId_t threadId)
{
	taskLock.writeAcquire();

	if (tasks[PROCID_THREAD(threadId)] != NULL)
	{
		delete tasks[PROCID_THREAD(threadId)];
		tasks[PROCID_THREAD(threadId)] = NULL;
	};

	nTasks--;
	taskLock.writeRelease();
}

