#ifndef _WRAP_AROUND_COUNTER_H
	#define _WRAP_AROUND_COUNTER_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>

class wrapAroundCounterC
{
public:
	wrapAroundCounterC(void);
	error_t initialize(uarch_t maxval);

public:
	sarch_t getNextValue(void **arr);

private:
	sarch_t		maxVal;
	sharedResourceGroupC<waitLockC, sarch_t>	nextVal;
};


/**	Inline methods.
 ****************************************************************************/

inline wrapAroundCounterC::wrapAroundCounterC(uarch_t maxVal)
{
	nextVal.rsrc = 0;
}

inline error_t wrapAroundCounterC::initialize(uarch_t maxVal)
{
	wrapAroundCounterC::maxVal = static_cast<srach_t>( maxVal );
}

inline sarch_t wrapAroundCounterC::getNextValue(void **arr)
{
	sarch_t		ret;

	nextVal.lock.acquire();

	ret = nextVal.rsrc;

	for (sarch_t i=((nextVal.rsrc == -1) ? 0 : nextVal.rsrc);
		i<=maxVal;
		i++)
	{
		if (arr[i] == __KNULL)
		{
			nextVal.rsrc = i;
			nextVal.lock.release();
			return ret;
		};
	};

	nextVal.rsrc = -1;
	nextVal.lock.release();
	return ret;
}

