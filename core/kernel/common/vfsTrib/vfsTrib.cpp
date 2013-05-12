
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kstdlib/__kclib/stdlib.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <kernel/common/vfsTrib/vfsTrib.h>


status_t vfs::getIndexOfNext(
	utf8Char *path, utf8Char splitChar, uarch_t maxLength)
{
	uarch_t		i;

	for (i=0; path[i] != '\0' && i<maxLength; i++) {
		if (path[i] == splitChar) { return i; };
	};

	if (i == maxLength) { return ERROR_INVALID_RESOURCE_NAME; };
	return ERROR_NOT_FOUND;
}

