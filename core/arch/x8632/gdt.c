
#include <arch/x8632/gdt.h>


/**	CAVEAT:
 * Do not try to rename this file to have the extension .cpp, or try to build
 * the kernel by running a compiler in "C++" parsing mode for this file. C++
 * compilers have the habit of not hardcoding structs in the output objects,
 * and instead generating constructors to fill out hardcoded structs with their
 * values at runtime.
 *
 * However, the x86-32 GDT is used by the kernel long before we ever run
 * constructors; therefore, if some C++ compiler doesn't properly hardcode the
 * GDT entries as seen in here, your build will fail to run.
 **/

struct sX8632GdtEntry		x8632Gdt[] =
{
	{0, 0, 0, 0, 0, 0},
	// Kernel descriptors.
	{0xFFFF, 0, 0, 0x9A, 0xCF, 0},
	{0xFFFF, 0, 0, 0x92, 0xCF, 0},
	// Userspace descriptors.
	{0xFFFF, 0, 0, 0xFA, 0xCF, 0},
	{0xFFFF, 0, 0, 0xF2, 0xCF, 0},
	// Space for 8 LDTs. Use the LDTs to hold TSSs.
	{0,0,0,0,0, 0},
	{0,0,0,0,0, 0},
	{0,0,0,0,0, 0},
	{0,0,0,0,0, 0}
};

struct sX8632GdtPtr		x8632GdtPtr =
{
	(sizeof(struct sX8632GdtEntry) * 9) - 1,
	x8632Gdt
};

