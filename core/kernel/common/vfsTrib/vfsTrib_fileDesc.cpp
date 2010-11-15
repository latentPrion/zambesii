
#include <__kstdlib/__kclib/string8.h>
#include <__kstdlib/__kcxxlib/new>
#include <kernel/common/vfsTrib/vfsTrib.h>
#include <kernel/common/timerTrib/timeTypes.h>


vfsFileC::vfsFileC(void)
{
	name = __KNULL;
	parent = __KNULL;
	next = __KNULL;
	desc = __KNULL;

	type = VFSDESC_TYPE_FILE;
	flags = 0;
	refCount = 0;
}

vfsFileC::vfsFileC(vfsFileInodeC *inode)
{
	name = __KNULL;
	parent = __KNULL;
	next = __KNULL;
	desc = inode;

	type = VFSDESC_TYPE_FILE;
	flags = 0;
	refCount = 0;	
}

error_t vfsFileC::initialize(
	utf8Char *fileName,
	ubit32 inodeLow, ubit32 inodeHigh, uarch_t fileSize
	)
{
	ubit32		nameLen;
	error_t		ret;

	// Allocate space for name.
	nameLen = strlen8(fileName);
	if (nameLen > VFSFILE_NAME_MAX_NCHARS) {
		nameLen = VFSFILE_NAME_MAX_NCHARS;
	};

	name = new utf8Char[nameLen];
	if (name == __KNULL) {
		return ERROR_MEMORY_NOMEM;
	};

	strncpy8(name, fileName, nameLen);
	name[nameLen] = '\0';

	// Allocate inode.
	if (desc == __KNULL)
	{
		desc = new vfsFileInodeC(inodeHigh, inodeLow, fileSize);
		if (desc == __KNULL) {
			return ERROR_MEMORY_NOMEM;
		};
	};

	ret = desc->initialize();
	if (ret == ERROR_SUCCESS) {
		return ERROR_SUCCESS;
	};

	delete desc;
	desc = __KNULL;
	return ret;
}

vfsFileC::~vfsFileC(void)
{
	delete name;
	if (desc != __KNULL && desc->refCount <= 0) {
		delete desc;
	};
}

