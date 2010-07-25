#ifndef _SWAMP_INFO_NODE_H
	#define _SWAMP_INFO_NODE_H

	#include <__kstdlib/__ktypes.h>

class swampInfoNodeC
{
public:
	uarch_t			startAddr, nPages;
	swampInfoNodeC		*prev, *next;

public:
	inline sarch_t operator ==(swampInfoNodeC &sin);
	inline sarch_t operator ==(int v);
	inline sarch_t operator !=(int v);
	inline sarch_t operator >(swampInfoNodeC &sin);
	inline sarch_t operator <(swampInfoNodeC &sin);
	inline sarch_t operator <=(swampInfoNodeC &sin);
	inline sarch_t operator >=(swampInfoNodeC &sin);
	// inline swampInfoNodeC &operator =(swampInfoNodeC &sin);
	inline swampInfoNodeC &operator =(int v);
};


/**	Inline Methods
 ********************************************************************/

sarch_t swampInfoNodeC::operator ==(swampInfoNodeC &sin)
{
	return startAddr == sin.startAddr;
}

sarch_t swampInfoNodeC::operator ==(int v)
{
	if (v == 0) {
		return startAddr == 0;
	};
	return 0;
}

sarch_t swampInfoNodeC::operator !=(int v)
{
	if (v == 0) {
		return v != 0;
	};
	return 0;
}

sarch_t swampInfoNodeC::operator >(swampInfoNodeC &sin)
{
	return startAddr > sin.startAddr;
}

sarch_t swampInfoNodeC::operator <(swampInfoNodeC &sin)
{
	return startAddr < sin.startAddr;
}

sarch_t swampInfoNodeC::operator <=(swampInfoNodeC &sin)
{
	return startAddr <= sin.startAddr;
}

sarch_t swampInfoNodeC::operator >=(swampInfoNodeC &sin)
{
	return startAddr >= sin.startAddr;
}

/*
swampInfoNodeC &swampInfoNodeC::operator =(swampInfoNodeC &sin)
{
	next = sin.next; prev = sin.prev;
	startAddr = sin.startAddr;
	nPages = sin.nPages;
	return *this;
}
*/

swampInfoNodeC &swampInfoNodeC::operator =(int v)
{
	if (v == 0) {
		startAddr = 0;
	};
	return *this;
}

#endif

