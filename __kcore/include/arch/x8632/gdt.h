#ifndef _x86_32_GDT_H
	#define _x86_32_GDT_H

	#include <__kstdlib/__ktypes.h>

struct sX8632GdtEntry
{
	ubit16	limitLow;
	ubit16	baseLow;
	ubit8	baseMid;
	ubit8	opts;
	ubit8	gran;
	ubit8	baseHigh;
} __attribute__((packed));

struct sX8632GdtPtr
{
	ubit16		limit;
	struct sX8632GdtEntry	*baseAddr;
} __attribute__((packed));

extern struct sX8632GdtPtr	x8632GdtPtr;
#endif

