
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kclib/string16.h>
#include <__kstdlib/__kclib/stdlib.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <kernel/common/vfsTrib/vfsTrib.h>


vfsTribC::vfsTribC(void)
:
inodeStack(VFSTRIB_INODE_STACK_NITEMS)
{
	// Loop the root to point to itself.
	root.parent = &root;
	root.desc = &trees;
	// Root cannot have any siblings.
	root.next = __KNULL;
	root.flags = 0;
	root.type = 0;
	strcpy16(root.name, (utf16Char *)"");
	inodeCounter = 0;
}

vfsTribC::~vfsTribC(void)
{
}

error_t vfsTribC::initialize(void)
{
	error_t		ret;

	ret = inodeStack.initialize();
	if (ret != ERROR_SUCCESS) {
		return ret;
	};

	ret = dirInodeHash.initialize();
	if (ret != ERROR_SUCCESS) {
		return ret;
	};

	ret = fileInodeHash.initialize();
	if (ret != ERROR_SUCCESS) {
		return ret;
	};

	fileDescCache = cachePool.createCache(sizeof(vfsFileC));
	if (fileDescCache == __KNULL) {
		return ERROR_MEMORY_NOMEM;
	};

	dirDescCache = cachePool.createCache(sizeof(vfsDirC));
	if (dirDescCache == __KNULL) {
		return ERROR_MEMORY_NOMEM;
	};

	return ERROR_SUCCESS;
}

error_t vfsTribC::getNewInode(ubit32 *inodeLow)
{
	// Try to pop an inode number off the stack.
	if (inodeStack.pop(inodeLow) == ERROR_SUCCESS) {
		return ERROR_SUCCESS;
	};

	// Else use the inode counter.
	if (inodeCounter == 0xFFFFFFFF) {
		return ERROR_CRITICAL;
	};

	*inodeLow = inodeCounter++;
	return ERROR_SUCCESS;
}

void vfsTribC::releaseDirInode(ubit32 inodeLow)
{
	inodeStack.push(inodeLow);
}

