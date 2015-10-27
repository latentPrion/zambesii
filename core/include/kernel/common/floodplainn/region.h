#ifndef _FLOODPLAINN_UDI_REGION_H
	#define _FLOODPLAINN_UDI_REGION_H

	#define UDI_VERSION	0x101
	#include <udi.h>
	#undef UDI_VERSION
	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/heapList.h>

class Thread;

namespace fplainn
{
	class RegionEndpoint;
	class DeviceInstance;

	class Region
	{
	public:
		error_t initialize(void)
			{ return endpoints.initialize(); }

		void dumpChannelEndpoints(void);

		sbit8 findEndpoint(RegionEndpoint *endp)
		{
			HeapList<RegionEndpoint>::Iterator	it;

			it = endpoints.begin(0);
			for (; it != endpoints.end(); ++it)
			{
				if (*it != endp) { continue; };
				return 1;
			};

			return 0;
		}

	private:
		friend class RegionEndpoint;

		error_t addEndpoint(RegionEndpoint *endp)
			{ return endpoints.insert(endp); }

		sbit8 removeEndpoint(RegionEndpoint *endp)
			{ return endpoints.remove(endp); }

	public:
		DeviceInstance		*parent;
		ubit16			index;
		Thread			*thread;
		HeapList<RegionEndpoint>	endpoints;
	};
}

#endif
