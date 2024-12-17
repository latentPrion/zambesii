
#include <arch/x8632/idt.h>
#include <arch/x8632/interrupts.h>

struct sX8632IdtEntry		x8632Idt[256];
struct sX8632IdtPtr		x8632IdtPtr =
{
	(sizeof(struct sX8632IdtEntry) * 256) - 1,
	x8632Idt
};

