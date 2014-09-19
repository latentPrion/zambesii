#ifndef _FLOODPLAINN_UDI_REGION_H
	#define _FLOODPLAINN_UDI_REGION_H

	#define UDI_VERSION	0x101
	#include <udi.h>
	#undef UDI_VERSION
	#include <__kstdlib/__ktypes.h>

class Thread;

namespace fplainn
{
	class DeviceInstance;

	class Region
	{
	public:
		DeviceInstance		*parent;
		ubit16			index;
		Thread			*thread;
		udi_init_context_t	*rdata;
	};
}

#endif
