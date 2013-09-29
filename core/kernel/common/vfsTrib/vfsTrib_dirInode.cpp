
#include <__kstdlib/__kclib/string8.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <kernel/common/vfsTrib/vfsTrib.h>
#include <kernel/common/timerTrib/timeTypes.h>


vfsDirInodeC::vfsDirInodeC(ubit32 _inodeHigh, ubit32 _inodeLow)
:
inodeLow(_inodeLow), inodeHigh(_inodeHigh)
{
	// Inode ID as extracted from underlying concrete FS.
	nSubDirs = nFiles = 0;
	refCount = 0;

	files.rsrc = NULL;
	subDirs.rsrc = NULL;
}

error_t vfsDirInodeC::initialize(void)
{
	return ERROR_SUCCESS;
}

void vfsDirInodeC::dumpSubDirs(void)
{
	vfsDirC		*curDir;

	subDirs.lock.acquire();

	__kprintf(NOTICE"Subdirs: %d.\n", nSubDirs);
	for (curDir = subDirs.rsrc; curDir != NULL; curDir = curDir->next)
	{
		__kprintf(NOTICE"\tName: %s, Refcount: %d.\n",
			curDir->name, curDir->refCount);
	};

	subDirs.lock.release();
}

void vfsDirInodeC::dumpFiles(void)
{
	vfsFileC	*curFile;

	files.lock.acquire();

	__kprintf(NOTICE"Files: %d.\n", nFiles);
	for (curFile = files.rsrc; curFile != NULL; curFile = curFile->next)
	{
		__kprintf(NOTICE"\tName: %s, Refcount: %d.\n",
			curFile->name, curFile->refCount);
	};

	files.lock.release();
}

void vfsDirInodeC::addFileDesc(vfsFileC *newFile)
{
	if (newFile == NULL) { return; };

	// Pretty much linked list manipulation. Nothing interesting.
	files.lock.acquire();

	newFile->next = files.rsrc;
	files.rsrc = newFile;
	nFiles++;

	files.lock.release();
}

void vfsDirInodeC::addDirDesc(vfsDirC *newDir)
{
	if (newDir == NULL) { return; };

	// Same here.
	subDirs.lock.acquire();

	newDir->next = subDirs.rsrc;
	subDirs.rsrc = newDir;
	nSubDirs++;

	subDirs.lock.release();
}

status_t vfsDirInodeC::removeFileDesc(utf8Char *name)
{
	vfsFileC		*curFile, *prevFile;

	prevFile = NULL;

	files.lock.acquire();

	curFile = files.rsrc;
	for (; curFile != NULL; )
	{
		// If the folder exists:
		if (strcmp8(curFile->name, name) == 0)
		{
			if (prevFile != NULL) {
				prevFile->next = curFile->next;
			}
			else {
				files.rsrc = curFile->next;
			};
			nFiles--;

			files.lock.release();

			curFile->~vfsFileC();
			vfsTrib.fileDescCache->free(curFile);
			return ERROR_SUCCESS;
		};

		prevFile = curFile;
		curFile = curFile->next;
	};

	// Folder didn't exist.
	files.lock.release();
	return VFSPATH_INVALID;
}

status_t vfsDirInodeC::removeDirDesc(utf8Char *name)
{
	vfsDirC		*curDir, *prevDir;

	prevDir = NULL;

	subDirs.lock.acquire();

	curDir = subDirs.rsrc;
	for (; curDir != NULL; )
	{
		// If the folder exists:
		if (strcmp8(curDir->name, name) == 0)
		{
			if (prevDir != NULL) {
				prevDir->next = curDir->next;
			}
			else {
				subDirs.rsrc = curDir->next;
			};
			nSubDirs--;

			subDirs.lock.release();

			curDir->~vfsDirC();
			vfsTrib.dirDescCache->free(curDir);
			return ERROR_SUCCESS;
		};

		prevDir = curDir;
		curDir = curDir->next;
	};

	// Folder didn't exist.
	subDirs.lock.release();
	return VFSPATH_INVALID;
}

vfsFileC *vfsDirInodeC::getFileDesc(utf8Char *name)
{
	vfsFileC	*curFile;

	files.lock.acquire();

	curFile = files.rsrc;
	for (uarch_t i=0; i<nFiles && curFile != NULL; i++)
	{
		if (strcmp8(curFile->name, name) == 0)
		{
			files.lock.release();
			return curFile;
		};
		curFile = curFile->next;
	};

	// File doesn't exist on this inode.
	files.lock.release();
	return NULL;
}

vfsDirC *vfsDirInodeC::getDirDesc(utf8Char *name)
{
	vfsDirC		*curDir;

	subDirs.lock.acquire();

	curDir = subDirs.rsrc;
	for (; curDir != NULL; )
	{
		if (strcmp8(curDir->name, name) == 0)
		{
			subDirs.lock.release();
			return curDir;
		};
		curDir = curDir->next;
	};

	// Subfolder doesn't exist.
	subDirs.lock.release();
	return NULL;
}

vfsDirInodeC::~vfsDirInodeC(void)
{
}

