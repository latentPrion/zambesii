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
#define ZMESSAGE_SUBSYSTEM_MAXVAL		(0x1)
#define ZMESSAGE_SUBSYSTEM_TIMER		(0x0)
#define ZMESSAGE_SUBSYSTEM_PROCESS		(0x1)

namespace zrequest
{
	#define ZREQUEST_FLAGS_CPU_SOURCE		(1<<0)
	#define ZREQUEST_FLAGS_CPU_TARGET		(1<<1)

	// Common header contained within all request messages.
	struct headerS
	{
		processId_t	sourceId, targetId;
		void		*privateData;
		ubit32		flags;
		ubit16		subsystem, function;
	};

	/* Generic request message type. Can be used where there is no need for
	 * a specialized message format.
	 **/
	struct genericS
	{
		headerS		header;
	};
}

namespace zcallback
{
	/**	Flags for headerS::flags
	 **/
	#define ZCALLBACK_FLAGS_CPU_SOURCE		(1<<0)
	#define ZCALLBACK_FLAGS_CPU_TARGET		(1<<1)
	struct headerS
	{
		/**	EXPLANATION:
		 * err:
		 *	Most API callbacks return an error status; this
		 *	member reduces the need for custom messages by
		 *	enabling callbacks to use it instead of
		 *	specialized messages.
		 **/
		processId_t	sourceId;
		void		*privateData;
		ubit32		flags;
		error_t		err;
		ubit16		subsystem, function;
	};

	/* Generic callback header. Can be used where no special message type is
	 * needed.
	 **/
	struct genericS
	{
		headerS		header;
	};
}

#endif

