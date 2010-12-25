#ifndef _WRAP_AROUND_COUNTER_H
	#define _WRAP_AROUND_COUNTER_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>

class wrapAroundCounterC
{
public:
	wrapAroundCounterC(sarch_t maxVal);
	error_t initialize(void);

public:
	sarch_t getNextValue(void **arr, ubit8 secondTry=0);

private:
	sarch_t		maxVal;
	sharedResourceGroupC<waitLockC, sarch_t>	nextVal;
};


/**	Inline methods.
 ****************************************************************************/

inline wrapAroundCounterC::wrapAroundCounterC(sarch_t maxVal)
{
	nextVal.rsrc = 0;
	wrapAroundCounterC::maxVal = static_cast<sarch_t>( maxVal );
}

inline error_t wrapAroundCounterC::initialize(void)
{
	return ERROR_SUCCESS;
}

inline sarch_t wrapAroundCounterC::getNextValue(void **arr, ubit8 secondTry)
{
	sarch_t		ret;

	nextVal.lock.acquire();

	for (sarch_t i=nextVal.rsrc; i<=maxVal;	i++)
	{
		if (arr[i] == __KNULL)
		{
			ret = i;
			nextVal.rsrc = i + 1;
			nextVal.lock.release();
			return ret;
		};
	};

	nextVal.rsrc = 0;
	nextVal.lock.release();
	return ((secondTry) ? (-1) : getNextValue(arr, 1));
}

#endif

