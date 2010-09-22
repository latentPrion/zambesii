#ifndef _LOCALE_TRIB_H
	#define _LOCALE_TRIB_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/tributary.h>

class localeTribC
:
public tributaryC
{
public:
	/* Returns a UTF-16 BOM which will indicate the endianness of the
	 * kernel's internal string processing. On a little endian arch, this
	 * will return a little endian BOM, and on a big-endian arch, it will
	 * return a big endian BOM.
	 **/
	utf16Char getSystemBom(void) { return 0xFEFF; };
};

#endif

