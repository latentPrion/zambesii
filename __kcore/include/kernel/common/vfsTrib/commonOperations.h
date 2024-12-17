#ifndef _VFS_COMMON_OPERATIONS_H
	#define _VFS_COMMON_OPERATIONS_H

	#include <__kstdlib/__ktypes.h>

namespace vfs
{
	class DirectoryOperations
	{
	public:
		virtual error_t openDirectory(utf8Char *name, void **handle)=0;
		virtual error_t readDirectory(void *handle, void **tagInfo)=0;
		virtual void closeDirectory(void *handle)=0;
	};
}

#endif

