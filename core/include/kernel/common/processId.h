#ifndef _PROCESS_ID_H
	#define _PROCESS_ID_H

	#include <arch/arch.h>
	#include <__kstdlib/__ktypes.h>

/**	EXPLANATION:
 * Organization of process IDs. A process ID is always a uarch_t. Therefore the
 * number of processes which may be handled by a build of the kernel is
 * determined by the word-size of the arch in question.
 *
 * We split up process IDs as such, until further notice:
 *
 * Half of the native word-sized integer goes to the process Id, and the other
 * half to the thread Id.
 *
 * I'm not sure how POSIX process groups will come to bear on this later on.
 * AFAICT, though, POSIX process groups are just groups of processes that are
 * spawned in such a manner as to allow them to recieve POSIX signals as a unit.
 **/

typedef uarch_t		processId_t;

#ifdef __32_BIT__

#define PROCID_PROCESS_MASK	0xFFFF
#define PROCID_PROCESS_SHIFT	16

#define PROCID_THREAD_MASK	0xFFFF
#define PROCID_THREAD_SHIFT	0

#define PROCID_INVALID		((processId_t)(0xFFFF << PROCID_PROCESS_SHIFT))

#elif defined(__64_BIT__)

#define PROCID_PROCESS_MASK	0xFFFFFFFF
#define PROCID_PROCESS_SHIFT	32

#define PROCID_THREAD_MASK	0xFFFFFFFF
#define PROCID_THREAD_SHIFT	0

#define PROCID_INVALID		\
	((processId_t)(0xFFFFFFFF << PROCID_PROCESS_SHIFT))

#endif

#define __KPROCESSID		(0x1 << PROCID_PROCESS_SHIFT)

#define PROCID_PROCESS(__id)				\
	(((__id) >> PROCID_PROCESS_SHIFT) & PROCID_PROCESS_MASK)

#define PROCID_THREAD(__id)				\
	(((__id) >> PROCID_THREAD_SHIFT) & PROCID_THREAD_MASK)

#define PROCID_TASK(__id)	PROCID_THREAD(__id)

#define PROCESSID_PROCESS(__id)		PROCID_PROCESS(__id)
#define PROCESSID_THREAD(__id)		PROCID_THREAD(__id)
#define PROCESSID_TASK(__id)		PROCID_THREAD(__id)

#endif

