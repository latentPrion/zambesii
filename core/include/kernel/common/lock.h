#ifndef _LOCK_H
	#define _LOCK_H

	#include <__kstdlib/__ktypes.h>

#define LOCK_FLAGS_IRQS_WERE_ENABLED		(1<<0)
// 10C1CO1C -> "lockOk". I'm not good with these magic numbers.
#define LOCK_MAGIC				0x10C1C01C

class Lock
{
public:
	Lock(void)
	{
		lock = 0;
		flags = 0;
		magic = LOCK_MAGIC;
	};

	struct operationDescriptorS
	{
		enum typeE { WAIT=0, RECURSIVE, MULTIPLE_READER };
		enum unlockOperationE { READ=0, WRITE };

		operationDescriptorS(Lock *lock, typeE type=WAIT)
		:
		lock(lock), type(type)
		{}

		operationDescriptorS(
			Lock *lock, typeE type,
			unlockOperationE op, uarch_t rwFlags)
		:
		lock(lock), type(type), operation(op), rwFlags(rwFlags)
		{}

		void execute(void);

		Lock			*lock;
		typeE			type;
		unlockOperationE	operation;
		uarch_t			rwFlags;
	};

	void setLock(void) { lock = 1; };
	void unlock(void) { lock = 0; };

#if __SCALING__ >= SCALING_SMP
	volatile sarch_t	lock;
#endif
protected:
	uarch_t			flags;

public:
	uarch_t			magic;
};

#endif

