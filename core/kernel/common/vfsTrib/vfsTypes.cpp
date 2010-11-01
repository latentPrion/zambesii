
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

	this->desc = new vfsFileInodeC;
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
	memset(name, 0, sizeof(utf16Char) * 128);
	type = 0;
	desc = 0;
	flags = 0;
}

error_t vfsDirC::initialize(void)
{
	error_t		ret;

	this->desc = new vfsDirInodeC;
	if (this->desc == __KNULL) {
		return ERROR_MEMORY_NOMEM;
	};

	ret = this->desc->initialize();
	if (ret != ERROR_SUCCESS) {
		delete this->desc;
	};

	return ret;
}

vfsFileInodeC::vfsFileInodeC(void)
{
	inodeLow = inodeHigh = 0;
	fileSize = 0;
}

error_t vfsFileInodeC::initialize(void)
{
	return ERROR_SUCCESS;
}

vfsDirInodeC::vfsDirInodeC(void)
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

error_t vfsDirInodeC::initialize(void)
{
	return ERROR_SUCCESS;
}

