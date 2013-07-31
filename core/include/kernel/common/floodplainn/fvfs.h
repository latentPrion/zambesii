#ifndef _FLOODPLAINN_VFS_H
	#define _FLOODPLAINN_VFS_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/vfsTrib/vfsTypes.h>

namespace fvfs
{
	enum deviceTypeE { ENUMERATOR=0, LEAF };

	typedef vfs::tagC	tagC;

	/**	hvfs::enumeratorC:
	 * Base type for a device that has child devices.
	 **********************************************************************/
	class enumeratorDeviceC
	:
	public vfs::dirInodeC
	{
	public:
		enumeratorC(void) {}
		error_t initialize(void)
			{ return vfs::dirInodeC::initialize(); }

		~enumeratorC(void) {}

	public:
		void *getDevice(void) { return &device; }

	private:
		deviceC		device;
	};
		
}

#endif

