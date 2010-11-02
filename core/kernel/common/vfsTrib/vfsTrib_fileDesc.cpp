
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

