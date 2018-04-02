#ifndef _RESIZEABLE_ID_ARRAY
	#define _RESIZEABLE_ID_ARRAY

	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/resizeableArray.h>
	#include <__kclasses/wrapAroundCounter.h>

/**	EXPLANATION:
 * This is a convenience class which enables you to maintain a resizeable array
 * of elements coupled with the ability to maintain unique IDs for each index
 * and not trample them.
 **/
template <class T>
class ResizeableIdArray
:	public ResizeableArray<T>
{
public:
	ResizeableIdArray(uarch_t maxVal)
	:	ResizeableArray<T>(),
	idCounter(maxVal)
	{}

	~ResizeableIdArray(void) {}

	error_t initialize(void)
	{
		ResizeableArray<T>::initialize();
		idCounter.initialize();
		return ERROR_SUCCESS;
	}

	sarch_t unlocked_getNextValue(ubit8 secondTry=0);

private:
	sarch_t getNextValue(void **arr, ubit8 secondTry=0)
	{
		return ERROR_UNIMPLEMENTED;
	}

public:
	WrapAroundCounter	idCounter;
};


/**	Inline methods:
 ******************************************************************************/

template <class T>
sarch_t ResizeableIdArray<T>::unlocked_getNextValue(ubit8 secondTry)
{
	return idCounter.getNextValue(
		reinterpret_cast<void **>(ResizeableArray<T>::s.rsrc.array),
		secondTry);
}

#endif
