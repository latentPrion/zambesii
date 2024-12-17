
#include <__kclasses/debugPipe.h>
#include <commonlibs/libzbzcore/libzbzcore.h>
#include <kernel/common/thread.h>
#include <kernel/common/process.h>
#include <kernel/common/vfsTrib/vfsTrib.h>
#include <kernel/common/taskTrib/taskTrib.h>


void __klzbzcore::distributary::main(Thread *self, mainCbFn *callback)
{
	dvfs::Tag		*tmpTag;
	error_t			ret;
	void			(*jumpAddress)(void);

	ret = vfsTrib.getDvfs()->getPath(self->parent->fullName, &tmpTag);
	if (ret != ERROR_SUCCESS)
	{
		printf(ERROR LZBZCORE"dtribPath: Failed to resolve dtrib name. "
			"\n\t(%s).\n",
			self->parent->fullName);
	};

	callback(self, ret);

	jumpAddress = tmpTag->getDInode()->getEntryAddress();
	(*jumpAddress)();
}
