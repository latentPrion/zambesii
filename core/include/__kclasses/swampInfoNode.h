#ifndef _SWAMP_INFO_NODE_H
	#define _SWAMP_INFO_NODE_H

	#include <__kstdlib/__ktypes.h>

class swampInfoNodeC
{
public:
	swampInfoNodeC(
		void *baseAddr=__KNULL, uarch_t nPages=0,
		swampInfoNodeC *prev=__KNULL, swampInfoNodeC *next=__KNULL)
	:
	baseAddr(baseAddr), nPages(nPages),
	prev(prev), next(next)
	{}

public:
	void			*baseAddr;
	uarch_t			nPages;
	swampInfoNodeC		*prev, *next;

public:
	inline sarch_t operator ==(swampInfoNodeC &sin);
	inline sarch_t operator ==(void *v);
	inline sarch_t operator !=(void *v);
	inline sarch_t operator >(swampInfoNodeC &sin);
	inline sarch_t operator <(swampInfoNodeC &sin);
	inline sarch_t operator <=(swampInfoNodeC &sin);
	inline sarch_t operator >=(swampInfoNodeC &sin);
	// inline swampInfoNodeC &operator =(swampInfoNodeC &sin);
	inline swampInfoNodeC &operator =(void *);
};


/**	Inline Methods
 ********************************************************************/

sarch_t swampInfoNodeC::operator ==(swampInfoNodeC &sin)
{
	return baseAddr == sin.baseAddr;
}

sarch_t swampInfoNodeC::operator ==(void *v)
{
	if (v == __KNULL) {
		return baseAddr == __KNULL;
	};
	return 0;
}

sarch_t swampInfoNodeC::operator !=(void *v)
{
	if (v == __KNULL) {
		return baseAddr != __KNULL;
	};
	return 0;
}

sarch_t swampInfoNodeC::operator >(swampInfoNodeC &sin)
{
	return baseAddr > sin.baseAddr;
}

sarch_t swampInfoNodeC::operator <(swampInfoNodeC &sin)
{
	return baseAddr < sin.baseAddr;
}

sarch_t swampInfoNodeC::operator <=(swampInfoNodeC &sin)
{
	return baseAddr <= sin.baseAddr;
}

sarch_t swampInfoNodeC::operator >=(swampInfoNodeC &sin)
{
	return baseAddr >= sin.baseAddr;
}

/*
swampInfoNodeC &swampInfoNodeC::operator =(swampInfoNodeC &sin)
{
	next = sin.next; prev = sin.prev;
	baseAddr = sin.baseAddr;
	nPages = sin.nPages;
	return *this;
}
*/

swampInfoNodeC &swampInfoNodeC::operator =(void *v)
{
	if (v == __KNULL) {
		baseAddr = __KNULL;
	};
	return *this;
}

#endif

