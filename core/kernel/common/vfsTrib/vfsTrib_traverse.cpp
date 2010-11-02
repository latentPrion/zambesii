
#include <__kstdlib/__kclib/string16.h>
#include <kernel/common/vfsTrib/vfsTrib.h>


vfsFileC *vfsTribC::getFileDesc(vfsDirInodeC *inode, utf16Char *name)
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

vfsDirC *vfsTribC::getDirDesc(vfsDirInodeC *inode, utf16Char *name)
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

