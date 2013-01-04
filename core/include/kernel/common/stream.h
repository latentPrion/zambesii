#ifndef _STREAM_C_H
	#define _STREAM_C_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>

/**	EXPLANATION:
 * StreamC is a fundamental class in Zambezii which is essential to the
 * Zambezii functional model. The stream is the basic concept which aids
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
 * dynamic, which is very original and specific to Zambezii:
 *
 * If one cuts a process's link to its stream, it is effectively cut off
 * from syscalls for that resource. I.e: cutting a thread's networking
 * related stream cuts it off from all networking activity, for example.
 **/

class streamC
{
public:
	explicit streamC(uarch_t id)
	:
	id(id), flags(0)
	{
		binding.rsrc = 0;
	};

	streamC(void)
	:
	id(0), flags(0)
	{
		binding.rsrc = 0;
	};

public:
	virtual void bind(void) {};
	virtual void cut(void) {};

// jumpListC interface.
public:
	uarch_t		id;
	ubit32		flags;

protected:
	sharedResourceGroupC<waitLockC, ubit8>	binding;
};

#endif
