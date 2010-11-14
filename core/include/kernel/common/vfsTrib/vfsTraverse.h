#ifndef _VFS_TRAVERSE_AUX_FUNCTIONS_H
	#define _VFS_TRAVERSE_AUX_FUNCTIONS_H

	#include <__kstdlib/__ktypes.h>

namespace vfsTraverse
{
	sarch_t isPathAlias(utf16Char *path);
	sarch_t isTree(utf16Char *path);
	sarch_t isUnixRoot(utf16Char *path);

	sbit32 getNextSegmentIndex(utf16Char *path);
	status_t validateSegment(utf16Char *path);

	vfsFileC *getFileDesc(vfsDirInodeC *inode, utf16Char *name);
	vfsDirC *getDirDesc(vfsDirInodeC *inode, utf16Char *name);

	status_t getRelativePath(
		vfsDirC *dir, utf16Char *path, ubit8 *type, void **ret);
}


/**	Inline methods
 *****************************************************************************/

inline sarch_t vfsTraverse::isPathAlias(utf16Char *path)
{
	return (path[1] == ':' && ((path[0] >= 'A' && path[0] <= 'Z')
		|| (path[0] >= 'a' && path[0] <= 'z')));
}

inline sarch_t vfsTraverse::isTree(utf16Char *path)
{
	return (path[0] == ':');
}

inline sarch_t vfsTraverse::isUnixRoot(utf16Char *path)
{
	return (path[0] == '/');
}

#endif

