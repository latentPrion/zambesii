#ifndef _x86_EMU_MEMORY_AUXILIARY_FUNCTIONS_H
	#define _x86_EMU_MEMORY_AUXILIARY_FUNCTIONS_H

	#include "x86emu.h"

#ifdef __cplusplus
#define XEAEXTERN	extern "C"
#else
#define XEAEXTERN	extern
#endif

XEAEXTERN X86EMU_pioFuncs	x86Emu_ioFuncs;
XEAEXTERN X86EMU_memFuncs	x86Emu_memFuncs;

#endif

