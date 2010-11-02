
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kcxxlib/new>
#include <kernel/common/vfsTrib/vfsTrib.h>
#include <kernel/common/timerTrib/timeTypes.h>


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

vfsDirC::~vfsDirC(void)
{
	delete this->desc;
}

