
#include <arch/x8632/idt.h>
#include <arch/x8632/interrupts.h>

struct x8632IdtEntryS		x8632Idt[256];
struct x8632IdtPtrS		x8632IdtPtr =
{
	(sizeof(struct x8632IdtEntryS) * 256) - 1,
	x8632Idt
};

