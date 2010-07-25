#ifndef _LOCK_H
	#define _LOCK_H

	#include <__kstdlib/__ktypes.h>

#define LOCK_FLAGS_IRQS_WERE_ENABLED		(1<<0)

class lockC
{
	public:
		lockC(void)
		{
			lock = 0;
			flags = 0;
		};

	protected:
#if __SCALING__ >= SCALING_SMP
		volatile sarch_t	lock;
#endif
		uarch_t			flags;
};

#endif

