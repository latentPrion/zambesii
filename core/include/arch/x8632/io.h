#ifndef _x86_32_IO_H
	#define _x86_32_IO_H

	#include <__kstdlib/__ktypes.h>

namespace io
{
	inline ubit8 read8(ubit16 port);
	inline ubit16 read16(ubit16 port);
	inline ubit32 read32(ubit16 port);

	inline void write8(ubit16 port, ubit8 val);
	inline void write16(ubit16 port, ubit16 val);
	inline void write32(ubit16 port, ubit32 val);
}


/**	Inline methods
 ****************************************************************************/

inline ubit8 io::read8(ubit16 port)
{
	ubit8	ret;
	asm volatile(
		"inb %1,%0"
		:"=a"(ret)
		:"Nd"(port));
	return ret;
}

inline ubit16 io::read16(ubit16 port)
{
	ubit16	ret;
	asm volatile(
		"inw %1,%0"
		:"=a"(ret)
		:"Nd"(port));
	return ret;
}

inline ubit32 io::read32(ubit16 port)
{
	ubit32	ret;
	asm volatile(
		"inl %1,%0"
		:"=a"(ret)
		:"Nd"(port));
	return ret;
}

inline void io::write8(ubit16 port, ubit8 val)
{
	asm volatile("outb %0,%1"
	:
	:"a"(val), "Nd" (port));
}

inline void io::write16(ubit16 port, ubit16 val)
{
	asm volatile("outw %0,%1"
	:
	:"a"(val), "Nd" (port));
}

inline void io::write32(ubit16 port, ubit32 val)
{
	asm volatile("outl %0,%1"
	:
	:"a"(val), "Nd" (port));
}

#endif

