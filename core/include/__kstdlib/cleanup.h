#ifndef ___KLOCAL_CLEANUP_RAII_H
	#define ___KLOCAL_CLEANUP_RAII_H

	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kcxxlib/new>

#define LOCALCLEANUP_MAGIC		(0xABADC0DE)

class vaddrSpaceStreamC;
class memoryStreamC;

namespace cleanup
{
	struct		pointerS
	{
		pointerS &operator =(void *p)
		{
			v = p;
			return *this;
		}

		void		*v;
		status_t	nPages;
	};

	void vmemFree(
		int idx, pointerS *pointers, int nPointers,
		vaddrSpaceStreamC *vasStream=NULL);

	void vmemUnmapAndFree(
		int idx, pointerS *pointers, int nPointers,
		vaddrSpaceStreamC *vasStream=NULL);

	void streamFree(
		int idx, pointerS *pointers, int nPointers,
		memoryStreamC *memStream=NULL);
}

/**	EXPLANATION:
 * A very bad substitute for std::unique_ptr<>. I'll eventually get an
 * implementation of std::unique_ptr<> for pointers whose memory comes from the
 * heap.
 *
 * The advantage that this class offers is that it also handles properly
 * disposing of loose vmem pages allocated with vaddrSpaceStreamC::getPages()
 * and memoryStreamC::memAlloc().
 *
 * Ideally, I should create custom std::unique_ptr<>s for those. Eventually I
 * will, but this will suffice for now.
 *
 * Basically, you initialize an instance of this class, and write a custom
 * destructor per function it is used in. When the function exits, the
 * destructor is called, and the memory is released as you dictated. You need to
 * specify what type of operation should be used to destroy the pointer. E.g.:
 *	heapFree() for heap pointers,
 *	vmemFree() for vaddrSpaceStreamC::getPages() loose vmem pages.
 *	streamFree() for memoryStreamC::memAlloc() pages.
 **/
template <int nPointers>
class cleanupC
{
public:
	cleanupC(void)
	{
		for (sarch_t i=0; i<nPointers; i++) {
			pointers[i].v = NULL;
		};
	}

	~cleanupC(void) {}

	cleanup::pointerS &operator [](int idx)
	{
		// Deliberately trigger a #PF with a magic write value.
		if (idx >= nPointers)
			{ *((uarch_t *)NULL) = LOCALCLEANUP_MAGIC; };

		return *(&pointers[idx]);
	}

	void heapFree(int idx) { delete (ubit8 *)pointers[idx].v; }
	void vmemFree(int idx, vaddrSpaceStreamC *vasStream=NULL)
		{ cleanup::vmemFree(idx, pointers, nPointers, vasStream); }

	void vmemUnmapAndFree(int idx, vaddrSpaceStreamC *vasStream=NULL)
	{
		cleanup::vmemUnmapAndFree(idx, pointers, nPointers, vasStream);
	}

	void streamFree(int idx, memoryStreamC *memStream=NULL)
		{ cleanup::streamFree(idx, pointers, nPointers, memStream); };

protected:
	cleanup::pointerS	pointers[nPointers];
};

#define DONT_SEND_RESPONSE		((void *)NULL)

/**	EXPLANATION:
 * Basically, you give this class a pointer to a block of memory which holds
 * an asynchronous response message. The message will automatically be sent when
 * the an instance of the class destructs, UNLESS the internal pointer is
 * NULL. If the internal pointer is NULL, the message will not be sent.
 *
 * To set the internal message pointer, use:
 *	object.setMessage(POINTER_TO_BLOCK_OF_MEMORY).
 * To set the error value that is set in the message's header, use:
 *	object.setMessage(YOUR_CHOSEN_ERROR_VALUE).
 *
 * You can also use operator() as a shorthand:
 *	object(POINTER_TO_BLOCK_OF_MEMORY) or,
 *	object(YOUR_CHOSEN_ERROR_VALUE).
 *
 * If you already have a message pointer stored internally from a previous
 * setMessage(), you can clear it by calling:
 *	setMessage(DONT_SEND_RESPONSE), which is the same as
 *	setMessage((void *)NULL);
 **/
class asyncResponseC
:
public cleanupC<1>
{
public:
	~asyncResponseC(void);

	void operator() (error_t e) { setMessage(e); }
	void operator() (void *msg) { setMessage(msg); }

	void setMessage(error_t e)
		{ pointers[0].nPages = e; }

	void setMessage(void *msg)
		{ pointers[0].v = msg; }
};

#endif

