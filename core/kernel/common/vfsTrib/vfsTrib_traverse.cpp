
#include <__kstdlib/__kclib/string16.h>
#include <kernel/common/vfsTrib/vfsTrib.h>
#include <kernel/common/vfsTrib/vfsTraverse.h>


sbit32 vfsTraverse::getNextSegmentIndex(utf16Char *path)
{
	sbit32		ret=0;

	// Return the string index of the character after the next '/'.
	for (; path[ret] != 0; ret++)
	{
		if (path[ret] == '/') {
			return ret+1;
		};
	};
	return -1;
}

status_t vfsTraverse::validateSegment(utf16Char *segment)
{
	/**	EXPLANATION:
	 * VFS only reserves '\0', ':' and '/'. Underlying filesystems are
	 * required to scan for any further reserved characters they need to
	 * make sure are absent. ':' is reserved since it's used to identify
	 * both path aliases ("C:") and root trees (":mytree").
	 *
	 * We don't need to actually check for '\0' since it acts as a
	 * terminator, so any '\0' will automatically end the parsing. We also
	 * don't need to check for '/' since it's the path separator, and
	 * any attempt to place it in a string will just be interpreted as a
	 * path separator in the string.
	 *
	 * Apart from that, any character is valid in the VFS.
	 **/
	for (; *segment != 0; segment++)
	{
		// Add any new reserved characters here later on.
		switch (*segment)
		{
		case ':':
			return VFSPATH_INVALID_CHAR;
		default:
			continue;
		};
	};
	return ERROR_SUCCESS;
}

status_t vfsTraverse::getRelativePath(
	vfsDirC *dir, utf16Char *path, ubit8 *type, void **ret
	)
{
	vfsFileC	*file;
	sbit32		idx;

	/**	EXPLANATION:
	 * Just recursively keep splitting the path at '/', and trying to get
	 * a folder with the name of the segment, until you reach the last
	 * path segment.
	 **/
	for (; *path != '\0'; )
	{
		idx = getNextSegmentIndex(path);
		// Temporarily replace next '/' with '\0'.
		if (idx >= 0) {
			path[idx - 1] = '\0';
		};

		dir = getDirDesc(dir->desc, path);
		if (dir == __KNULL)
		{
			if (idx >= 0)
			{
				path[idx - 1] = '/';
				return VFSPATH_INVALID;
			};

			// Else is last segment in path, could be file name.
			file = getFileDesc(dir->desc, path);
			path[idx - 1] = '/';
			if (file == __KNULL) {
				return VFSPATH_INVALID;
			};

			*type = VFSPATH_TYPE_FILE;
			*ret = file;
			return ERROR_SUCCESS;
		};

		if (idx < 0) {
			*path = '\0';
		}
		else {
			path = &path[idx];
		};
	};

	*type = VFSPATH_TYPE_DIR;
	*ret = dir;
	return ERROR_SUCCESS;
}

vfsFileC *vfsTraverse::getFileDesc(vfsDirInodeC *inode, utf16Char *name)
{
	vfsFileC	*curFile;

	inode->files.lock.acquire();

	curFile = inode->files.rsrc;
	for (uarch_t i=0; i<inode->nFiles && curFile != __KNULL; i++)
	{
		if (strcmp16(curFile->name, name) == 0)
		{
			inode->files.lock.release();
			return curFile;
		};
		curFile = curFile->next;
	};

	// File doesn't exist on this inode.
	inode->files.lock.release();
	return __KNULL;
}

vfsDirC *vfsTraverse::getDirDesc(vfsDirInodeC *inode, utf16Char *name)
{
	vfsDirC		*curDir;

	inode->subDirs.lock.acquire();

	curDir = inode->subDirs.rsrc;
	for (uarch_t i=0; i<inode->nSubDirs && curDir != __KNULL; i++)
	{
		if (strcmp16(curDir->name, name) == 0)
		{
			inode->subDirs.lock.release();
			return curDir;
		};
		curDir = curDir->next;
	};

	// Subfolder doesn't exist.
	inode->subDirs.lock.release();
	return __KNULL;
}

