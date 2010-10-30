
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kcxxlib/new>
#include <kernel/common/vfsTrib/vfsTrib.h>
#include <kernel/common/timerTrib/timeTypes.h>


vfsFileC::vfsFileC(void)
{
	memset(this, 0, sizeof(*this));
}

error_t vfsFileC::initialize(void)
{
	error_t		ret;

	this->desc = new vfsFileDescC;
	if (this->desc == __KNULL) {
		return ERROR_MEMORY_NOMEM;
	};

	ret = this->desc->initialize();
	if (ret != ERROR_SUCCESS) {
		delete this->desc;
	};

	return ret;
}

vfsDirC::vfsDirC(void)
{
	memset(this, 0, sizeof(*this));
}

error_t vfsDirC::initialize(void)
{
	error_t		ret;

	this->desc = new vfsDirDescC;
	if (this->desc == __KNULL) {
		return ERROR_MEMORY_NOMEM;
	};

	ret = this->desc->initialize();
	if (ret != ERROR_SUCCESS) {
		delete this->desc;
	};

	return ret;
}

vfsFileDescC::vfsFileDescC(void)
{
	inodeLow = inodeHigh = 0;
	fileSize = 0;
}

error_t vfsFileDescC::initialize(void)
{
	return ERROR_SUCCESS;
}

vfsDirDescC::vfsDirDescC(void)
{
	inodeLow = inodeHigh = 0;
	nSubdirs = nFiles = 0;

	DATEU32_TO_DATE(createdDate, 0, 0, 0);
	DATEU32_TO_DATE(modifiedDate, 0, 0, 0);
	DATEU32_TO_DATE(accessedDate, 0, 0, 0);

	TIMEU32_TO_TIME(createdTime, 0, 0, 0, 0);
	TIMEU32_TO_TIME(modifiedTime, 0, 0, 0, 0);
	TIMEU32_TO_TIME(accessedTime, 0, 0, 0, 0);

	files.rsrc = __KNULL;
	subDirs.rsrc = __KNULL;
}

error_t vfsDirDescC::initialize(void)
{
	return ERROR_SUCCESS;
}

