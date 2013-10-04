#ifndef _KERNEL_COMMON_MESSAGES_H
	#define _KERNEL_COMMON_MESSAGES_H

	#include <__kstdlib/__ktypes.h>

/**	EXPLANATION:
 * All of these structures must remain POD (plain-old-data) types. They are used
 * as part of the userspace ABI. In general, meaning of the header members is
 * as follows:
 *
 * sourceId:
 *	ID of the CPU or thread that made the asynch call-in that prompted this
 *	asynch round-trip.
 * targetId (only for zrequest::headerS).
 *	ID of the target CPU or thread that the callback/response for this
 *	asynch round-trip must be sent to.
 * flags: (common subset):
 *	Z*_FLAGS_CPU_SOURCE: The source object was a CPU and not a thread.
 *	Z*_FLAGS_CPU_TARGET: The target object is a CPU and not a thread.
 * privateData:
 *	Opaque storage (sizeof(void *)) that is uninterpeted by the kernel
 *	subsystem that processes the request; this will be copied from the
 *	request object and into the callback object.
 * subsystem + function:
 *	Used by the source thread to determine which function call the callback
 *	is for. Most subsystems will not have more than 0xFFFF functions and
 *	those that do can overlap their functions into a new subsystem ID, such
 *	that they use more than one subsystem ID (while remaining a single
 *	subsystem).
 *
 *	Also, since every new subsystem ID implies a separate, fresh queue,
 *	subsystems which wish to isolate specific groups of functions into their
 *	own queues can overlap those functions into a new subsystem ID if so
 *	desired.
 **/

/**	Values for zrequest::headerS::subsystem & zcallback::headerS::subsystem.
 * Subsystem IDs are basically implicitly queue IDs (to the kernel; externally
 * no assumptions should be made about the mapping of subsystem IDs to queues
 * in the kernel).
 **/
// Keep this up to date.
#define ZMESSAGE_SUBSYSTEM_MAXVAL		(0x8)
// Actual subsystem values.
#define ZMESSAGE_SUBSYSTEM_TIMER		(0x0)
#define ZMESSAGE_SUBSYSTEM_PROCESS		(0x1)
#define ZMESSAGE_SUBSYSTEM_USER0		(0x2)
#define ZMESSAGE_SUBSYSTEM_USER1		(0x3)
#define ZMESSAGE_SUBSYSTEM_USER2		(0x4)
#define ZMESSAGE_SUBSYSTEM_USER3		(0x5)
#define ZMESSAGE_SUBSYSTEM_REQ0			(0x6)
#define ZMESSAGE_SUBSYSTEM_REQ1			(0x7)
#define ZMESSAGE_SUBSYSTEM_FLOODPLAINN		(0x8)

#define ZMESSAGE_USERQ(num)			(ZMESSAGE_SUBSYSTEM_USER0 + num)
#define ZMESSAGE_REQQ(num)			(ZMESSAGE_SUBSYSTEM_REQ0 + num)

class taskC;

namespace zmessage
{
	#define ZMESSAGE_FLAGS_CPU_SOURCE		(1<<0)
	#define ZMESSAGE_FLAGS_CPU_TARGET		(1<<1)

	// Common header contained within all messages.
	struct headerS
	{
		processId_t	sourceId, targetId;
		void		*privateData;
		error_t		error;
		ubit16		subsystem, flags;
		ubit32		function;
		uarch_t		size;
	};

	struct iteratorS
	{
		headerS		header;
		ubit8		_padding_[256];
	};

	processId_t determineSourceThreadId(taskC *caller, ubit16 *flags);
	processId_t determineTargetThreadId(
		processId_t targetId, processId_t sourceId,
		uarch_t callerFlags, ubit16 *messageFlags);
}

#endif

