#ifndef _x86_32_IDT_H
	#define _x86_32_IDT_H

	#include <__kstdlib/__ktypes.h>

struct x8632IdtEntryS
{
	ubit16		baseLow;
	ubit16		seg;
	ubit8		rsvd;
	ubit8		flags;
	ubit16		baseHigh;
} __attribute__((packed));

struct x8632IdtPtrS
{
	ubit16			limit;
	struct x8632IdtEntryS	*idt;
} __attribute__((packed));

#endif

