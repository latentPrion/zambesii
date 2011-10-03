#ifndef _LOCK_H
	#define _LOCK_H

	#include <__kstdlib/__ktypes.h>

#define LOCK_FLAGS_IRQS_WERE_ENABLED		(1<<0)
// 10C1CO1C -> "lockOk". I'm not good with these magic numbers.
#define LOCK_MAGIC				0x10C1C01C

class lockC
{
public:
	lockC(void)
	{
		lock = 0;
		flags = 0;
		magic = LOCK_MAGIC;
	};

	void setLock(void) { lock = 1; };
	void unlock(void) { lock = 0; };

protected:
#if __SCALING__ >= SCALING_SMP
	volatile sarch_t	lock;
#endif
	uarch_t			flags;

public:
	uarch_t			magic;
};

#endif

