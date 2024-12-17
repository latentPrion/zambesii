#ifndef ___K_VECTOR_H
	#define ___K_VECTOR_H

	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kflagManipulation.h>
	#include <__kstdlib/__kclib/stdlib.h>
	#include <kernel/common/panic.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>
	#include <kernel/common/memoryTrib/__kmemoryStream_methods.h>
	#include <kernel/common/memoryTrib/allocFlags.h>

	/**	CAVEAT:
	 * The only thing that is MP safe in this class is the act of resizing
	 * it. Everything else is *NOT* mp-safe.
	 **/

#define RESIZEABLE_ARRAY_FLAGS_NO_FAKEMAP	(1<<0)

template <class T>
class ResizeableArray
{
public:
	ResizeableArray(uarch_t _flags=0)
	:
	flags(_flags)
	{}

	error_t initialize(void) { return ERROR_SUCCESS; }

	~ResizeableArray(void)
	{
		s.rsrc.nIndexes = 0;
		::operator delete(s.rsrc.array);
	}

public:
	void lock(void) { s.lock.acquire(); }
	void unlock(void) { s.lock.release(); }

	uarch_t unlocked_getNIndexes(void)
	{
		return s.rsrc.nIndexes;
	}

	uarch_t getNIndexes(void)
	{
		uarch_t		ret;

		lock();
		ret = unlocked_getNIndexes();
		unlock();
		return ret;
	}

	T *getInternalArrayAddress(void)
	{
		return s.array;
	}

	sbit8 unlocked_indexIsValid(uarch_t index)
	{
		return index + 1 <= s.rsrc.nIndexes;
	}

	// This operator is unlocked.
	T &operator [](uarch_t index)
	{
		if (s.rsrc.array == NULL)
		{
			panic(
				ERROR_INVALID_STATE,
				CC"ARArray op[]: Internal array is NULL");
		};

		if (!unlocked_indexIsValid(index))
		{
			panic(
				ERROR_LIMIT_OVERFLOWED,
				CC"ARArray op[]: Index overflows array");
		};


		return s.rsrc.array[index];
	}

	sbit8 indexIsValid(uarch_t index)
	{
		uarch_t		tmp;

		tmp = getNIndexes();
		return index + 1 <= tmp;
	};

	enum resizeToHoldIndexFlagsE {
		RTHI_FLAGS_UNLOCKED = (1<<0)
	};
	error_t resizeToHoldIndex(uarch_t index, uarch_t _flags=0)
	{
		error_t		ret;

		if (!FLAG_TEST(_flags, RTHI_FLAGS_UNLOCKED)) { lock(); };

		if (unlocked_indexIsValid(index))
		{
			unlock();
			return ERROR_SUCCESS;
		};

		if (FLAG_TEST(this->flags, RESIZEABLE_ARRAY_FLAGS_NO_FAKEMAP))
		{
			// Else, resize array.
			ret = crudeRealloc(
				s.rsrc.array, sizeof(T) * s.rsrc.nIndexes,
				(void **)&s.rsrc.array,
				sizeof(T) * (index + 1));
		}
		else
		{
			void		*newmem;

			/**	EXPLANATION:
			 * The RESIZEABLE_ARRAY_FLAGS_NO_FAKEMAP flag was
			 * introduced to deal with the need for instances of
			 * ScatterGatherList to be able to be read/written
			 * inside of the PMM's constrainedGetFrames() method.
			 *
			 * Since any memory that is accessed by any layer of
			 * the memory management code must absolutely not
			 * page fault, we must ensure that all metadata
			 * manipulated by MM code is fully committed.
			 *
			 * Hence this feature: it enables ScatterGatherList to
			 * use ResizeableArray internally to store the list of
			 * frames that make up the SGList and ensure that this
			 * metadata does not trigger a page fault.
			 **/
			newmem = __kmemoryStream_memRealloc(
				s.rsrc.array, sizeof(T) * s.rsrc.nIndexes,
				sizeof(T) * (index + 1),
				MEMALLOC_NO_FAKEMAP);

			if (newmem != NULL)
			{
				ret = ERROR_SUCCESS;
				s.rsrc.array = static_cast<T *>(newmem);
			}
			else {
				ret = ERROR_MEMORY_NOMEM;
			}
		}

		if (ret == ERROR_SUCCESS) {
			s.rsrc.nIndexes = index + 1;
		};

		if (!FLAG_TEST(_flags, RTHI_FLAGS_UNLOCKED)) { unlock(); };

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

		lock();
		memmove(
			&s.rsrc.array[index], &s.rsrc.array[index + nItems],
			sizeof(*s.rsrc.array) * nItems);

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
		return Iterator(this, unlocked_getNIndexes());
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

	uarch_t						flags;
	SharedResourceGroup<WaitLock, sState>		s;
};

#endif
