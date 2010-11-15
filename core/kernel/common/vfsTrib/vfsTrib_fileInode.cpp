
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kcxxlib/new>
#include <kernel/common/vfsTrib/vfsTrib.h>
#include <kernel/common/timerTrib/timeTypes.h>


vfsFileInodeC::vfsFileInodeC(
	ubit32 _inodeHigh, ubit32 _inodeLow, uarch_t _fileSize
	)
:
inodeLow(_inodeLow), inodeHigh(_inodeHigh)
{
	refCount = 0;
	fileSize = _fileSize;
}

error_t vfsFileInodeC::initialize(void)
{
	return ERROR_SUCCESS;
}

vfsFileInodeC::~vfsFileInodeC(void)
{
}

