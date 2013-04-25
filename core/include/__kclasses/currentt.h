#ifndef _CURRENTT_H
	#define _CURRENTT_H

	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kclib/string8.h>

namespace currentt
{
	template <ubit16 maxNameLength>
	class vfsTagC
	{
	protected:
		vfsTagC(utf8Char *name)
		:
		flags(0)
		{
			strncpy8(this->name, name, maxNameLength);
			this->name[maxNameLength-1] = '\0';
		}

		error_t initialize(void) { return ERROR_SUCCESS; }

		~vfsTagC(void) {};

	public:
		utf8Char *getName(void) { return name; }
		ubit16 getMaxNameLength(void) { return maxNameLength; }

		status_t rename(utf8Char *newName);

	private:
		utf8Char		name[maxNameLength];
		ubit32			flags;
	};

	class vfsInodeC
	{
	protected:
		error_t initialize(void) { return ERROR_SUCCESS; }
	};

}

#endif

