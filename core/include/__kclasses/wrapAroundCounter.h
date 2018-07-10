#ifndef _WRAP_AROUND_COUNTER_H
	#define _WRAP_AROUND_COUNTER_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>

class WrapAroundCounter
{
public:
	WrapAroundCounter(sarch_t maxVal, uarch_t startVal=0)
	:
	maxVal(maxVal)
	{
		nextVal.rsrc = startVal;
	};

	error_t initialize(void) { return ERROR_SUCCESS; };

public:
	template <class T>
	sarch_t getNextValue(T *arr, ubit8 secondTry=0);
	void setMaxVal(sarch_t newMaxVal)
	{
		maxVal = newMaxVal;
	}

private:
	sarch_t					maxVal;
	SharedResourceGroup<WaitLock, sarch_t>	nextVal;
};


/**	Inline methods.
 ****************************************************************************/

template <class T>
inline sarch_t WrapAroundCounter::getNextValue(T *arr, ubit8 secondTry)
{
	sarch_t		ret;

	nextVal.lock.acquire();

	for (sarch_t i=nextVal.rsrc; i<=maxVal; i++)
	{
		if (arr[i] == NULL)
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
