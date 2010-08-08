#ifndef _x86_EMU_AUXILIARY_FUNCS_H
	#define _x86_EMU_AUXILIARY_FUNCS_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/firmwareTrib/firmwareSupportRivApi.h>

/**	EXPLANATION:
Data structure containing pointers to programmed I/O functions used by the
emulator. This is used so that the user program can hook all programmed
I/O for the emulator to handled as necessary by the user program. By
default the emulator contains simple functions that do not do access the
hardware in any way. To allow the emulator access the hardware, you will
need to override the programmed I/O functions using the X86EMU_setupPioFuncs
function.

MEMBERS:
inb		- Function to read a byte from an I/O port
inw		- Function to read a word from an I/O port
inl     - Function to read a dword from an I/O port
outb	- Function to write a byte to an I/O port
outw    - Function to write a word to an I/O port
outl    - Function to write a dword to an I/O port
****************************************************************************/
typedef struct {
	u8  	(X86APIP inb)(X86EMU_pioAddr addr);
	u16 	(X86APIP inw)(X86EMU_pioAddr addr);
	u32 	(X86APIP inl)(X86EMU_pioAddr addr);
	void 	(X86APIP outb)(X86EMU_pioAddr addr, u8 val);
	void 	(X86APIP outw)(X86EMU_pioAddr addr, u16 val);
	void 	(X86APIP outl)(X86EMU_pioAddr addr, u32 val);
	} X86EMU_pioFuncs;

