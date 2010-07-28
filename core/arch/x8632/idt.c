
#include <arch/x8632/idt.h>
#include <arch/x8632/interrupts.h>

x8632IdtEntryS		x8632Idt[256];
x8632IdtPtrS		x8632IdtPtr =
{
	(sizeof(x8632IdtEntryS) * 256) - 1,
	x8632Idt
};

