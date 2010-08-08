
#include <arch/io.h>
#include <kernel/common/firmwareTrib/rivIoApi.h>

ubit8 io_read8(ubit32 port)
{
	return io::read8(port);
}

ubit16 io_read16(ubit32 port)
{
	return io::read16(port);
}

ubit32 io_read32(ubit32 port)
{
	return io::read32(port);
}

void io_write8(ubit32 port, ubit8 val)
{
	io::write8(port, val);
}

void io_write16(ubit32 port, ubit16 val)
{
	io::write16(port, val);
}

void io_write32(ubit32 port, ubit32 val)
{
	io::write32(port, val);
}

