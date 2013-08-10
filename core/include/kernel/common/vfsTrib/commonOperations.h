#ifndef _VFS_COMMON_OPERATIONS_H
	#define _VFS_COMMON_OPERATIONS_H

	#include <__kstdlib/__ktypes.h>

namespace vfs
{
	class directoryOperationsC
	{
	public:
		error_t openDirectory(utf8Char *name, void **handle)=0;
		error_t readDirectory(void *handle, void **tagInfo)=0;
		void closeDirectory(void *handle)=0;
	};
}

#endif

