
#include <__kstdlib/__kclib/string8.h>
#include <__kstdlib/__kcxxlib/new>
#include <kernel/common/vfsTrib/vfsTrib.h>
#include <kernel/common/timerTrib/timeTypes.h>


vfsDirC::vfsDirC(vfsDirINode *inode)
{
	desc = inode;
	next = NULL;	
	parent = NULL;
	name = NULL;

	type = VFSDESC_TYPE_DIR;
	refCount = 0;
	flags = 0;
}

vfsDirC::vfsDirC(void)
{
	desc = NULL;
	next = NULL;	
	parent = NULL;
	name = NULL;

	type = VFSDESC_TYPE_DIR;
	refCount = 0;
	flags = 0;
}

error_t vfsDirC::initialize(
	utf8Char *dirName, ubit32 inodeHigh, ubit32 inodeLow
	)
{
	error_t		ret;
	ubit32		nameLen;

	// Allocate space for the directory name.
	nameLen = strlen8(dirName);
	if (nameLen > VFSDIR_NAME_MAX_NCHARS) {
		nameLen = VFSDIR_NAME_MAX_NCHARS;
	};

	name = new utf8Char[nameLen + 1];
	if (name == NULL) {
		return ERROR_MEMORY_NOMEM;
	};
	strncpy8(name, dirName, nameLen);
	name[nameLen] = '\0';

	// Allocate a new inode if needed.
	if (desc == NULL)
	{
		desc = new vfsDirINode(inodeHigh, inodeLow);
		if (desc == NULL) {
			return ERROR_MEMORY_NOMEM;
		};
	};

	ret = desc->initialize();
	if (ret == ERROR_SUCCESS) {
		return ERROR_SUCCESS;
	};

	delete desc;
	desc = NULL;
	return ret;
}

status_t vfsDirC::rename(utf8Char *newName)
{
	utf8Char	*mem, *mem2;
	ubit32		newNameLen;

	if (strcmp8(name, newName) == 0) {
		return ERROR_SUCCESS;
	};

	newNameLen = strlen8(newName);
	if (newNameLen > VFSDIR_NAME_MAX_NCHARS) {
		newNameLen = VFSDIR_NAME_MAX_NCHARS;
	};

	mem = new utf8Char[newNameLen + 1];
	if (mem == NULL) {
		return ERROR_MEMORY_NOMEM;
	};

	strncpy8(mem, newName, newNameLen);
	mem[newNameLen] = '\0';

	mem2 = name;
	name = mem;

	delete mem2;
	return ERROR_SUCCESS;
}

vfsDirC::~vfsDirC(void)
{
	delete name;

	if (desc != NULL && desc->refCount <= 0) {
		delete desc;
	};
}

