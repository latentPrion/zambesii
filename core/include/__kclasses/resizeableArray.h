#ifndef ___K_VECTOR_H
	#define ___K_VECTOR_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/panic.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>

	/**	CAVEAT:
	 * The only thing that is MP safe in this class is the act of resizing
	 * it. Everything else is *NOT* mp-safe.
	 **/

template <class T>
class ResizeableArray
{
public:
	ResizeableArray(void) {}

	error_t	initialize(void) { return ERROR_SUCCESS; }

	~ResizeableArray(void)
	{
		s.rsrc.nIndexes = 0;
		::operator delete(s.rsrc.array);
	}

public:
	void lock(void) { s.lock.acquire(); }
	void unlock(void) { s.lock.release(); }

	uarch_t getNIndexes(void)
	{
		uarch_t		ret;

		lock();
		ret = s.rsrc.nIndexes;
		unlock();
		return ret;
	}

	T *getInternalArrayAddress(void)
	{
		return s.array;
	}

	T &operator [](uarch_t index)
	{
		if (s.rsrc.array == NULL)
		{
			panic(
				ERROR_INVALID_STATE,
				CC"ARArray op[]: Internal array is NULL");
		};

		if (!indexIsValid(index))
		{
			panic(
				ERROR_LIMIT_OVERFLOWED,
				CC"ARArray op[]: Index overflows array");
		};


		return s.rsrc.array[index];
	}

	sbit8 unsafe_indexIsValid(uarch_t index) volatile const
	{
		return index + 1 <= s.rsrc.nIndexes;
	}

	sbit8 indexIsValid(uarch_t index)
	{
		uarch_t		tmp;

		tmp = getNIndexes();
		return index + 1 <= tmp;
	};

	error_t resizeToHoldIndex(uarch_t index)
	{
		error_t		ret;

		lock();

		if (unsafe_indexIsValid(index))
		{
			unlock();
			return ERROR_SUCCESS;
		};

		// Else, resize array.
		ret = crudeRealloc(
			s.rsrc.array, sizeof(T) * s.rsrc.nIndexes,
			(void **)&s.rsrc.array, sizeof(T) * (index + 1));

		if (ret == ERROR_SUCCESS) {
			s.rsrc.nIndexes = index + 1;
		};

		unlock();

		return ret;
	}

	void erase(uarch_t index, uarch_t nItems)
	{
		if (s.rsrc.array == NULL)
		{
			panic(
				ERROR_INVALID_STATE,
				CC"ARArray erase: Internal array is NULL");
		};

		if (!indexIsValid(index) || !indexIsValid(index + nItems - 1))
		{
			panic(
				ERROR_LIMIT_OVERFLOWED,
				CC"ARArray erase: Index overflows array");
		};

		memmove(
			&s.rsrc.array[index], &s.rsrc.array[index + nItems],
			sizeof(*s.rsrc.array) * nItems);

		lock();
		s.rsrc.nIndexes -= nItems;
		unlock();
	}

public:
	class Iterator
	{
	public:
		Iterator(ResizeableArray *_parent=NULL, uarch_t _index=0)
		:
		parent(_parent), index(_index)
		{}

		~Iterator(void) {}

	public:
		T &operator *(void) { return (*parent)[index]; };

		void operator ++(void)
			{ index++; }

		void operator --(void)
			{ if (index > 0) { index--; }; }

		int operator != (Iterator it)
			{ return index != it.index; }

		int operator == (Iterator it)
			{ return index == it.index; }

	private:
		ResizeableArray		*parent;
		uarch_t			index;
	};

	Iterator begin(uarch_t beginAtIndex=0)
	{
		return Iterator(this, beginAtIndex);
	}

	Iterator end(void)
	{
		return Iterator(this, getNIndexes());
	}

public:
	struct sState
	{
		sState(void)
		:
		array(NULL), nIndexes(0)
		{}

		T		*array;
		uarch_t		nIndexes;
	};

	SharedResourceGroup<WaitLock, sState>		s;
};

#endif
