#ifndef _x86_32_IDT_H
	#define _x86_32_IDT_H

	#include <__kstdlib/__ktypes.h>

struct sX8632IdtEntry
{
	ubit16		baseLow;
	ubit16		seg;
	ubit8		rsvd;
	ubit8		flags;
	ubit16		baseHigh;
} __attribute__((packed));

struct sX8632IdtPtr
{
	ubit16			limit;
	struct sX8632IdtEntry	*idt;
} __attribute__((packed));

extern struct sX8632IdtEntry		x8632Idt[];
extern struct sX8632IdtPtr		x8632IdtPtr;

#endif

