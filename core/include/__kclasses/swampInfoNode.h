#ifndef _SWAMP_INFO_NODE_H
	#define _SWAMP_INFO_NODE_H

	#include <__kstdlib/__ktypes.h>

class swampInfoNodeC
{
public:
	swampInfoNodeC(
		void *baseAddr=NULL, uarch_t nPages=0,
		swampInfoNodeC *prev=NULL, swampInfoNodeC *next=NULL)
	:
	baseAddr(baseAddr), nPages(nPages),
	prev(prev), next(next)
	{}

public:
	void			*baseAddr;
	uarch_t			nPages;
	swampInfoNodeC		*prev, *next;
};

#endif

