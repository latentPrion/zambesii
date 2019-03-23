
#define UDI_VERSION 0x101
#include <udi.h>
#undef UDI_VERSION

#include <__kstdlib/__kclib/assert.h>
#include <__kstdlib/__kclib/string.h>
#include <kernel/common/process.h>
#include <kernel/common/floodplainn/dma.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <commonlibs/libzbzcore/libzudi.h>

void lzudi::buf::MappedScatterGatherList::memset8(
	uarch_t offset, ubit8 value, uarch_t nBytes
	)
{
	ubit8		*offvaddr;

	assert_fatal(offset + nBytes <= PAGING_PAGES_TO_BYTES(nFrames));

	offvaddr = reinterpret_cast<ubit8 *>(vaddr);
	offvaddr += offset;
	memset(offvaddr, value, nBytes);
}

sarch_t lzudi::buf::MappedScatterGatherList::write(
	const void *inbuff, uarch_t offset, uarch_t nBytes
	)
{
	ubit8		*offvaddr;

	if (offset + nBytes > PAGING_PAGES_TO_BYTES(nFrames))
		{ return ERROR_INVALID_ARG_VAL; }

	offvaddr = reinterpret_cast<ubit8 *>(vaddr);
	offvaddr += offset;
	memcpy(offvaddr, inbuff, nBytes);
	return nBytes;
}

sarch_t lzudi::buf::MappedScatterGatherList::read(
	void *const mem, uarch_t offset, uarch_t nBytes
	)
{
	ubit8		*offvaddr;

	if (offset + nBytes > PAGING_PAGES_TO_BYTES(nFrames))
		{ return ERROR_INVALID_ARG_VAL; }

	offvaddr = reinterpret_cast<ubit8 *>(vaddr);
	offvaddr += offset;
	memcpy(mem, offvaddr, nBytes);
	return nBytes;
}

void lzudi::buf::MappedScatterGatherList::memmove(
	uarch_t destOff, uarch_t srcOff, uarch_t nBytes
	)
{
	uarch_t		currentNBytes;
	ubit8		*destvaddr, *srcvaddr;

	currentNBytes = PAGING_PAGES_TO_BYTES(nFrames);

	assert_fatal(destOff + nBytes <= currentNBytes);
	assert_fatal(srcOff + nBytes <= currentNBytes);

	destvaddr = srcvaddr = reinterpret_cast<ubit8 *>(vaddr);
	destvaddr += destOff;
	srcvaddr += srcOff;

	::memmove(destvaddr, srcvaddr, nBytes);
}

sbit8 lzudi::buf::MappedScatterGatherList::hasEnoughMemoryForWrite(
	Thread *currThread,
	uarch_t dst_off, uarch_t dst_len,
	uarch_t src_len,
	uarch_t *nBytesExcessRequiredForWrite
	)
{
	uarch_t		currNBytes;

	assert_fatal(nBytesExcessRequiredForWrite != NULL);

	// Does the buf have enough memory behind it to carry out the write()?
	nFrames = currThread->parent->floodplainnStream
		.getNFramesInScatterGatherList(sGListIndex);

	currNBytes = PAGING_PAGES_TO_BYTES(nFrames);

	if (src_len > dst_len)
	{
		if (dst_off + src_len > currNBytes)
		{
			*nBytesExcessRequiredForWrite = (dst_off + src_len)
				- currNBytes;

			return 0;
		}
	}
	else
	{
		if (dst_off + dst_len > currNBytes)
		{
			*nBytesExcessRequiredForWrite = (dst_off + dst_len)
				- currNBytes;

			return 0;
		}
	}

	return 1;
}

sbit8 lzudi::buf::MappedScatterGatherList::hasEnoughMemoryForRead(
	Thread *currThread,
	uarch_t off, uarch_t len, uarch_t *nBytesExcessRequiredForWrite
	)
{
	uarch_t		currNBytes;

	assert_fatal(nBytesExcessRequiredForWrite != NULL);

	// Does the buf have enough memory behind it to carry out the write()?
	nFrames = currThread->parent->floodplainnStream
		.getNFramesInScatterGatherList(sGListIndex);

	currNBytes = PAGING_PAGES_TO_BYTES(nFrames);

	if (off + len > currNBytes)
	{
		*nBytesExcessRequiredForWrite = (off + len) - currNBytes;
		return 0;
	}

	return 1;
}

error_t lzudi::buf::allocateScatterGatherList(
	fplainn::dma::constraints::Compiler *comCons,
	uarch_t initialNBytes,
	MappedScatterGatherList **retobj
	)
{
	MappedScatterGatherList			*msgl;
	error_t					ret;
	Thread					*currThread;

	if (retobj == NULL || comCons == NULL) { return ERROR_INVALID_ARG; }

	currThread = cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentThread();

	/**	EXPLANATION:
	 * We want to allocate a ScatterGatherList object inside the kernel
	 * and constrain it with the constraints in "cons".
	 **/

	msgl = new MappedScatterGatherList;
	if (msgl == NULL) {
		return ERROR_MEMORY_NOMEM;
	}

	msgl->sGListIndex = currThread->parent->floodplainnStream
		.allocateScatterGatherList(NULL);

	if (msgl->sGListIndex < 0)
	{
		printf(ERROR LZUDI"buf:allocSGList(%d): allocSGList syscall "
			"failed.\n",
			initialNBytes);

		ret = msgl->sGListIndex;
		goto out_freeMsgl;
	}

	ret = currThread->parent->floodplainnStream
		.constrainScatterGatherList(msgl->sGListIndex, comCons);

	if (ret != ERROR_SUCCESS)
	{
		printf(ERROR LZUDI"buf:allocSGList(%d): constrain failed.\n",
			initialNBytes);

		goto out_freeSgl;
	}

	*retobj = msgl;
	return ERROR_SUCCESS;

out_freeSgl:
	currThread->parent->floodplainnStream.releaseScatterGatherList(
		msgl->sGListIndex);

out_freeMsgl:
	delete msgl;
	return ret;
}

void udi_buf_write(
	udi_buf_write_call_t *callback,
	udi_cb_t *gcb,
	const void *src_mem,
	udi_size_t src_len,
	udi_buf_t *dst_buf,
	udi_size_t dst_off,
	udi_size_t dst_len,
	udi_buf_path_t path_handle
	)
{
	error_t						err;
	lzudi::buf::MappedScatterGatherList		*msgl;
	uarch_t						currNFrames;
	Thread						*currThread;
	fplainn::dma::constraints::Compiler		requestedConstraints;

	LZUDI_CHECK_GCB_AND_CALLBACK_VALID(
		gcb, callback,
		gcb, NULL);

	currThread = cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentThread();

	if (dst_buf == NULL)
	{

		// path_handle must be supplied when allocating a buf.
		if (UDI_HANDLE_IS_NULL(path_handle, udi_path_handle_t))
		{
			printf(ERROR"UDI_BUF_ALLOC: A path handle must be "
				"supplied to enable constraint.\n");

			callback(gcb, NULL);
			return;
		}

		/**	FIXME:
		 * Ask the kernel for the constraints associated with the
		 * path handle.
		 **/
		err = currThread->parent->floodplainnStream.
			getParentConstraints(
				reinterpret_cast<uintptr_t>(path_handle),
				&requestedConstraints);

		if (err != ERROR_SUCCESS)
		{
			printf(ERROR LZUDI"BUF_ALLOC: Failed to get "
				"constraints for new buffer.\n");

			callback(gcb, NULL);
			return;
		}

		// Allocate a new SGList.
		err = lzudi::buf::allocateScatterGatherList(
			&requestedConstraints,
			src_len, &msgl);

		if (err != ERROR_SUCCESS)
		{
			printf(ERROR LZUDI"BUF_ALLOC: Failed to malloc "
				"handle.\n");

			callback(gcb, NULL);
			return;
		}

		// udi_buf_t is the base class so that we can downcast.
		dst_buf = static_cast<udi_buf_t *>(msgl);
	}

	// Upcast it into a MappedSGList.
	msgl = static_cast<lzudi::buf::MappedScatterGatherList *>(dst_buf);

	// Does the buf have enough memory behind it to carry out the write()?
	msgl->nFrames = currThread->parent->floodplainnStream
		.getNFramesInScatterGatherList(msgl->sGListIndex);
}
