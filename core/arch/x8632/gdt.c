
#include <arch/x8632/gdt.h>

struct x8632GdtEntryS		x8632Gdt[] =
{
	{0, 0, 0, 0, 0},
	// Kernel descriptors.
	{0xFFFF, 0, 0, 0x9A, 0xCF},
	{0xFFFF, 0, 0, 0x92, 0xCF},
	// Userspace descriptors.
	{0xFFFF, 0, 0, 0xFA, 0xCF},
	{0xFFFF, 0, 0, 0xF2, 0xCF},
	// Space for 8 LDTs. Use the LDTs to hold TSSs.
	{0,0,0,0,0},
	{0,0,0,0,0},
	{0,0,0,0,0},
	{0,0,0,0,0}
};

struct x8632GdtPtrS		x8632GdtPtr =
{
	(sizeof(struct x8632GdtEntryS) * 9) - 1,
	x8632Gdt
};

