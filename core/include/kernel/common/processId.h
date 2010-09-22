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

#define __KPROCESSID		0x0

#ifdef __32_BIT__

#define PROCID_GROUP_MASK	0xFF
#define PROCID_GROUP_SHIFT	24

#define PROCID_PROCESS_MASK	0xFFF
#define PROCID_PROCESS_SHIFT	12

#define PROCID_THREAD_MASK	0xFFF
#define PROCID_THREAD_SHIFT	0

#elif defined(__64_BIT__)

#define PROCID_GROUP_MASK	0xFFFF
#define PROCID_GROUP_SHIFT	48

#define PROCID_PROCESS_MASK	0xFFFFFF
#define PROCID_PROCESS_SHIFT	24

#define PROCID_THREAD_MASK	0xFFFFFF
#define PROCID_THREAD_SHIFT	0

#endif

#define PROCID_GROUP(__id)				\
	(((__id) >> PROCID_GROUP_SHIFT) & PROCID_GROUP_MASK)

#define PROCID_PROCESS(__id)			\
	(((__id) >> PROCID_PROCESS_SHIFT) & PROCID_PROCESS_MASK)

#define PROCID_THREAD(__id)				\
	(((__id) >> PROCID_THREAD_SHIFT) & PROCID_THREAD_MASK)

#endif

