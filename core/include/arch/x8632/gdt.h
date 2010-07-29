#ifndef _x86_32_GDT_H
	#define _x86_32_GDT_H

	#include <__kstdlib/__ktypes.h>

struct x8632GdtEntryS
{
	ubit16	limitLow;
	ubit16	baseLow;
	ubit8	baseMid;
	ubit8	opts;
	ubit8	gran;
	ubit8	baseHigh;
} __attribute__((packed));

struct x8632GdtPtrS
{
	ubit16		limit;
	struct x8632GdtEntryS	*baseAddr;
} __attribute__((packed));

#endif

