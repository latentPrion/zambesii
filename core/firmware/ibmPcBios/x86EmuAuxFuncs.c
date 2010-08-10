
#include <kernel/common/firmwareTrib/rivIoApi.h>
#include "x86EmuAuxFuncs.h"

static u8 inb(X86EMU_pioAddr addr)
{
	return io_read8(addr);
}

static u16 inw(X86EMU_pioAddr addr)
{
	return io_read16(addr);
}

static u32 inl(X86EMU_pioAddr addr)
{
	return io_read32(addr);
}

void outb(X86EMU_pioAddr addr, u8 val)
{
	io_write8(addr, val);
}

void outw(X86EMU_pioAddr addr, u16 val)
{
	io_write16(addr, val);
}

void outl(X86EMU_pioAddr addr, u32 val)
{
	io_write32(addr, val);
}

X86EMU_pioFuncs		x86Emu_ioFuncs =
{
	&inb,
	&inw,
	&inl,
	&outb,
	&outw,
	&outl
};


static ubit8 x86Emu_read8(ubit32 a)
{
	return *(ubit8 *)((uarch_t)a + (uarch_t)M.mem_base);
}

static ubit16 x86Emu_read16(ubit32 a)
{
	return *(ubit16 *)((uarch_t)a + (uarch_t)M.mem_base);
}

static ubit32 x86Emu_read32(ubit32 a)
{
	return *(ubit32 *)((uarch_t)a + (uarch_t)M.mem_base);
}

static void x86Emu_write8(ubit32 a, ubit8 v)
{
	*(ubit8 *)((uarch_t)a + (uarch_t)M.mem_base) = v;
}

static void x86Emu_write16(ubit32 a, ubit16 v)
{
	*(ubit16 *)((uarch_t)a + (uarch_t)M.mem_base) = v;
}

static void x86Emu_write32(ubit32 a, ubit32 v)
{
	*(ubit32 *)((uarch_t)a + (uarch_t)M.mem_base) = v;
}

X86EMU_memFuncs		x86Emu_memFuncs =
{
	&x86Emu_read8,
	&x86Emu_read16,
	&x86Emu_read32,
	&x86Emu_write8,
	&x86Emu_write16,
	&x86Emu_write32
};

