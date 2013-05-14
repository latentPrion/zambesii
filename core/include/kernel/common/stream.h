#ifndef _STREAM_C_H
	#define _STREAM_C_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>
	#include <kernel/common/processId.h>

/**	EXPLANATION:
 * StreamC is a fundamental class in Zambesii which is essential to the
 * Zambesii functional model. The stream is the basic concept which aids
 * in parallelism, and preemption, since it is essentially a form of
 * per-process or per-thread data.
 *
 * Streams, since they are per-process, can be used to monitor a
 * process, and more than that: to control a process or thread
 * individually from userspace. The idea is that for any kernel service
 * which involves syscalls, the syscall doesn't go to the kernel itself,
 * but is intercepted by the thread's stream, and the stream handles the
 * syscall on behalf of the thread.
 *
 * This way, the patterns of allocation, and usage of resources can be
 * accurately recorded. Also, the kernel can allow for something more
 * dynamic, which is very original and specific to Zambesii:
 *
 * If one cuts a process's link to its stream, it is effectively cut off
 * from syscalls for that resource. I.e: cutting a thread's networking
 * related stream cuts it off from all networking activity, for example.
 **/

class streamC
{
public:
	explicit streamC(processId_t id)
	:
	id(id), flags(0)
	{
		binding.rsrc = 0;
	};

public:
	virtual error_t bind(void) { return ERROR_SUCCESS; };
	virtual void cut(void) {};

// jumpListC interface.
public:
	processId_t	id;
	ubit32		flags;

protected:
	sharedResourceGroupC<waitLockC, ubit8>	binding;
};

#endif
