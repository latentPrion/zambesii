#ifndef __KERNEL_COMMON_DEADLOCK_DETECTION_H
	#define __KERNEL_COMMON_DEADLOCK_DETECTION_H

    #include <config.h>

#ifdef CONFIG_DEBUG_LOCKS

    #include <__kstdlib/__kclib/string.h>
    #include <kernel/common/waitLock.h>
    #include <kernel/common/sharedResourceGroup.h>

#define DEADLOCK_BUFF_MAX_NBYTES	(1024)
#define DEADLOCK_READ_MAX_NTRIES	(50000)
#define DEADLOCK_WRITE_MAX_NTRIES	(50000)

struct sDeadlockBuff
{
    sDeadlockBuff(void)
    :
    inUse(0)
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
