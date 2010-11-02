
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kcxxlib/new>
#include <kernel/common/vfsTrib/vfsTrib.h>
#include <kernel/common/timerTrib/timeTypes.h>


vfsFileInodeC::vfsFileInodeC(void)
{
	inodeLow = 0;
	fileSize = 0;
}

error_t vfsFileInodeC::initialize(void)
{
	return ERROR_SUCCESS;
}

