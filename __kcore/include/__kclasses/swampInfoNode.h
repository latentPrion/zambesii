#ifndef _SWAMP_INFO_NODE_H
	#define _SWAMP_INFO_NODE_H

	#include <__kstdlib/__ktypes.h>

class SwampInfoNode
{
public:
	SwampInfoNode(
		void *baseAddr=NULL, uarch_t nPages=0,
		SwampInfoNode *prev=NULL, SwampInfoNode *next=NULL)
	:
	baseAddr(baseAddr), nPages(nPages),
	prev(prev), next(next)
	{}

public:
	void			*baseAddr;
	uarch_t			nPages;
	SwampInfoNode		*prev, *next;
};

#endif

