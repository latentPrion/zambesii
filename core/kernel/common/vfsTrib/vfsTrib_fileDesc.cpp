
#include <__kstdlib/__kclib/string8.h>
#include <__kstdlib/__kcxxlib/new>
#include <kernel/common/vfsTrib/vfsTrib.h>
#include <kernel/common/timerTrib/timeTypes.h>


vfsFileC::vfsFileC(void)
{
	name = NULL;
	parent = NULL;
	next = NULL;
	desc = NULL;

	type = VFSDESC_TYPE_FILE;
	flags = 0;
	refCount = 0;
}

vfsFileC::vfsFileC(vfsFileInodeC *inode)
{
	name = NULL;
	parent = NULL;
	next = NULL;
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

	name = new utf8Char[nameLen + 1];
	if (name == NULL) {
		return ERROR_MEMORY_NOMEM;
	};

	strncpy8(name, fileName, nameLen);
	name[nameLen] = '\0';

	// Allocate inode.
	if (desc == NULL)
	{
		desc = new vfsFileInodeC(inodeHigh, inodeLow, fileSize);
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

vfsFileC::~vfsFileC(void)
{
	delete name;
	if (desc != NULL && desc->refCount <= 0) {
		delete desc;
	};
}

