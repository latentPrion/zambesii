#ifndef _LOCK_H
	#define _LOCK_H

	#include <config.h>
	#include <__kstdlib/__ktypes.h>

// 10C1CO1C -> "lockOk". I'm not good with these magic numbers.
#define LOCK_MAGIC				0x10C1C01C

class Lock
{
public:
	/** EXPLANATION:
	 * Flag shift values for lock::flags.
	 *
	 * We store the flags in the upper bits of the integer because
	 * MultipleReaderLock uses the lower bits to count the number of
	 * readers.
	 *
	 * We require at least 16 bits for the lower bits of lock::flags.
	 **/
	enum flagShiftE {
		FLAGS_ENUM_START = __UARCH_T_NBITS__ - (__BITS_PER_BYTE__),
		FLAGS_IRQS_WERE_ENABLED_SHIFT = FLAGS_ENUM_START,
		FLAGS_ENUM_END
	};

	// Compile-time check that we have at least 16 bits for lower bits of flags
	typedef char __lock_flags_size_check1[(__UARCH_T_NBITS__ - __BITS_PER_BYTE__ >= 16) ? 1 : -1];
	// Compile-time check that enum values fit within uarch_t
	typedef char __lock_flags_size_check2[(FLAGS_ENUM_END <= __UARCH_T_NBITS__) ? 1 : -1];

	// Actual flag values derived from shifts
	enum flagValueE {
		FLAGS_IRQS_WERE_ENABLED = (1 << FLAGS_IRQS_WERE_ENABLED_SHIFT)
	};

	Lock(void)
	{
		lock = 0;
		flags = 0;
		magic = LOCK_MAGIC;
	};

	struct sOperationDescriptor
	{
		enum typeE { WAIT=0, RECURSIVE, MULTIPLE_READER };
		enum unlockOperationE { READ=0, WRITE };

		sOperationDescriptor(Lock *lock, typeE type=WAIT)
		:
		lock(lock), type(type)
		{}

		sOperationDescriptor(
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
	volatile uarch_t	lock;
#endif
	uarch_t			magic;

protected:
	uarch_t			flags;
};

#endif

