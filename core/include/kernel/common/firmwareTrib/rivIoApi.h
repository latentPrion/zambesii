#ifndef _RIVULET_IO_API_H
	#define _RIVULET_IO_API_H

	#include <__kstdlib/__ktypes.h>

ubit8 io_read8(ubit32 port);
ubit16 io_read16(ubit32 port);
ubit32 io_read32(ubit32 port);

void io_write8(ubit32 port, ubit8 val);
void io_write16(ubit32 port, ubit16 val);
void io_write32(ubit32 port, ubit32 val);

#endif

