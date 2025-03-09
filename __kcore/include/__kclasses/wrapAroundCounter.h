#ifndef _WRAP_AROUND_COUNTER_H
	#define _WRAP_AROUND_COUNTER_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>

class WrapAroundCounter
{
public:
	WrapAroundCounter(sarch_t _maxVal, sarch_t _startVal=0)
	:
	startVal(_startVal)
	{
		sState		tmp = {
			_maxVal, startVal
		};

		state.rsrc = tmp;
	};

	error_t initialize(void) { return ERROR_SUCCESS; };

public:
	template <class T>
	uarch_t getSizeofT(T *) { return sizeof(T); }

	template <class T>
	sarch_t getNextValue(T *arr, ubit8 secondTry=0);

	void setMaxVal(sarch_t newMaxVal)
	{
		state.lock.acquire();
		state.rsrc.maxVal = newMaxVal;
		state.lock.release();
	}

private:
	struct sState {
		sarch_t		maxVal, nextVal;
	};
	sarch_t					startVal;
	SharedResourceGroup<WaitLock, sState>	state;
};


/**	Inline methods.
 ****************************************************************************/

template <class T>
inline sarch_t WrapAroundCounter::getNextValue(T *arr, ubit8 secondTry)
{
	sarch_t		ret;

	state.lock.acquire();

	for (sarch_t i=state.rsrc.nextVal; i<=state.rsrc.maxVal; i++)
	{
		if (arr[i] == NULL)
		{
			ret = i;
			state.rsrc.nextVal = i + 1;
			state.lock.release();
			return ret;
		};
	};

	state.rsrc.nextVal = startVal;
	state.lock.release();
	return ((secondTry) ? (-1) : getNextValue(arr, 1));
}

#endif
