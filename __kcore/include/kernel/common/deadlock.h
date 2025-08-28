#ifndef __KERNEL_COMMON_DEADLOCK_DETECTION_H
	#define __KERNEL_COMMON_DEADLOCK_DETECTION_H

    #include <config.h>

#ifdef CONFIG_DEBUG_LOCKS

    #include <__kstdlib/__kclib/string.h>
    #include <kernel/common/waitLock.h>
    #include <kernel/common/sharedResourceGroup.h>

#define DEADLOCK_BUFF_MAX_NBYTES	(1024 * 3)
#define DEADLOCK_READ_BASE_MAX_NTRIES	(100000)
#define DEADLOCK_WRITE_BASE_MAX_NTRIES	(100000)
#define DEADLOCK_PER_CPU_EXTRA_READ_NTRIES	(50000)
#define DEADLOCK_PER_CPU_EXTRA_WRITE_NTRIES	(50000)

inline uarch_t calcDeadlockNTries(
    uarch_t highestCpuId, uarch_t baseMaxTries,
	uarch_t perCpuExtraTries
    )
{
    /**EXPLANATION:
     * At early boot, the highest CPU ID is not known, so it defaults to
     * CPUID_INVALID, which is a negative number. We handle this case by
     * setting the scale factor to 1 until the highest CPU ID is known.
     **/
	uarch_t scaleFactor = (((signed)highestCpuId) <= 0) ? 1 : highestCpuId;
	return baseMaxTries + (scaleFactor * perCpuExtraTries);
}

inline uarch_t calcDeadlockNReadTries(uarch_t highestCpuId)
{
	return calcDeadlockNTries(highestCpuId, DEADLOCK_READ_BASE_MAX_NTRIES,
		DEADLOCK_PER_CPU_EXTRA_READ_NTRIES);
}

inline uarch_t calcDeadlockNWriteTries(uarch_t highestCpuId)
{
	return calcDeadlockNTries(highestCpuId, DEADLOCK_WRITE_BASE_MAX_NTRIES,
		DEADLOCK_PER_CPU_EXTRA_WRITE_NTRIES);
}

struct sDeadlockBuff
{
    sDeadlockBuff(void)
    :
    inUse(0), buffer(CC"DeadlockBuff buffer")
    {
        memset(buff, 0, sizeof(buff));
        buffer.rsrc = buff;
    }

    sbit8		inUse;
    utf8Char	buff[DEADLOCK_BUFF_MAX_NBYTES];
    SharedResourceGroup<WaitLock, utf8Char *>	buffer;
} extern deadlockBuffers[CONFIG_MAX_NCPUS];

/**
 * Common function to handle deadlock detection for all lock types
 *
 * @param formatStr Format string for the deadlock message
 * @param ... Variable arguments for the format string, including lock object, CPU ID, and caller address
 */
void reportDeadlock(utf8Char *formatStr, ...);

#endif

#endif /* __KERNEL_COMMON_DEADLOCK_DETECTION_H */
