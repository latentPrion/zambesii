
#include <arch/io.h>
#include "x86EmuAuxFuncs.h"


XEAEXTERN u8 inb(X86EMU_pioAddr addr);
XEAEXTERN u16 inw(X86EMU_pioAddr addr);
XEAEXTERN u32 inl(X86EMU_pioAddr addr);
XEAEXTERN void outb(X86EMU_pioAddr addr, u8 val);
XEAEXTERN void outw(X86EMU_pioAddr addr, u16 val);
XEAEXTERN void outl(X86EMU_pioAddr addr, u32 val);

u8 inb(X86EMU_pioAddr addr)
{
	return io::read8(addr);
}

u16 inw(X86EMU_pioAddr addr)
{
	return io::read16(addr);
}

u32 inl(X86EMU_pioAddr addr)
{
	return io::read32(addr);
}

void outb(X86EMU_pioAddr addr, u8 val)
{
	io::write8(addr, val);
}

void outw(X86EMU_pioAddr addr, u16 val)
{
	io::write16(addr, val);
}

void outl(X86EMU_pioAddr addr, u32 val)
{
	io::write32(addr, val);
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

