#ifndef _UDI_META_H
	#define _UDI_META_H

	#include <__kstdlib/__ktypes.h>

struct udiMetaDescS
{
	utf16Char	*name;
	utf16Char	*fileName;
	// Virtual address to which the loaded lib should be relocated.
	void		*baseAddr;
};

#endif

