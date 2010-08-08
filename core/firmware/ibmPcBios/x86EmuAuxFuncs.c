
#include "x86EmuAuxFuncs.h"

X86EMU_pioFuncs		x86Emu_ioFuncs
{
	&io_read8,
	&io_read16,
	&io_read32,
	&io_write8,
	&io_write16,
	&io_write32
}

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

X86EMU_memFuncs		x86Emu_memFuncs
{
	&x86Emu_read8,
	&x86Emu_read16,
	&x86Emu_read32,
	&x86Emu_write8,
	&x86Emu_write16,
	&x86Emu_write32
}

