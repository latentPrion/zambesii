
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
		printf(ERROR LZUDI"buf:allocSGList(): allocSGList syscall "
			"failed.\n");

		ret = msgl->sGListIndex;
		goto out_freeMsgl;
	}

	ret = currThread->parent->floodplainnStream
		.constrainScatterGatherList(msgl->sGListIndex, comCons);

	if (ret != ERROR_SUCCESS)
	{
		printf(ERROR LZUDI"buf:allocSGList(): constrain failed.\n");
		goto out_freeSgl;
	}

	*retobj = msgl;
	return ERROR_SUCCESS;

out_freeSgl:
	currThread->parent->floodplainnStream.releaseScatterGatherList(
		msgl->sGListIndex, 0);

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
	uarch_t						currNBytes,
							extraNBytesRequired;
	Thread						*currThread;
	sarch_t						status;

	/*	TODO:
	 * Don't forget to update dst_buf->buf_size.
	 */

	LZUDI_CHECK_GCB_AND_CALLBACK_VALID(gcb, callback, gcb, NULL);

	currThread = cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentThread();

	if (dst_buf == NULL)
	{
		// dst_buf == NULL means we need to allocate a new scgth list.

		// path_handle must be supplied when allocating a buf.
		if (UDI_HANDLE_IS_NULL(path_handle, udi_path_handle_t))
		{
			printf(ERROR LZUDI"BUF_ALLOC: A path handle must be "
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

	if (!msgl->hasEnoughMemoryForWrite(
		currThread, dst_off, dst_len, src_len, &extraNBytesRequired))
	{
		// Attempt to resize the SGList.
		err = currThread->parent->floodplainnStream
			.resizeScatterGatherList(
				msgl->sGListIndex,
				msgl->nFrames + PAGING_BYTES_TO_PAGES(
					extraNBytesRequired));

		if (err != ERROR_SUCCESS)
		{
			printf(ERROR LZUDI"BUF_WRITE: Failed to resize buf.\n");
			callback(gcb, NULL);
			return;
		}

		if (!msgl->hasEnoughMemoryForWrite(
			currThread,
			dst_off, dst_len, src_len, &extraNBytesRequired))
		{
			printf(ERROR LZUDI"BUF_WRITE: Resize returned success, "
				"yet still not enough memory for write.\n");

			callback(gcb, NULL);
			return;
		}

		err = currThread->parent->floodplainnStream
			.remapScatterGatherList(
				msgl->sGListIndex, &msgl->vaddr);

		if (err != ERROR_SUCCESS)
		{
			printf(ERROR LZUDI"BUF_WRITE: Remap after resize "
				"failed!\n");

			callback(gcb, NULL);
			return;
		}
	}

	if (src_mem == NULL || src_len == 0) {
		uarch_t		filler_len;

		/* If src_len == 0, then take dst_len;
		 * Else take the greater of src_len and dst_len.
		 * Effectively then, just take the greater of the two.
		 */
		filler_len = (src_len > dst_len) ? src_len : dst_len;
		msgl->memset8(dst_off, 0, filler_len);
	}
	else if (src_len > dst_len)
	{
		uarch_t filler_start, filler_len;
		/* For this case, the spec explicitly says to write past
		 * dst_len with "unspecified" bytes beyond src_len. So we write
		 * 0s.
		 */
		status = msgl->write(src_mem, dst_off, src_len);
		assert_fatal(status >= 0);

		filler_start = dst_off + src_len;
		filler_len = src_len - dst_len;
		msgl->memset8(filler_start, 0, filler_len);
	}
	else if (dst_len > src_len || dst_len == src_len)
	{
		/* For the case where dst_len > src_len the spec doesn't say
		 * what to do, so we only copy up to src_len and then return.
		 * I.e, we don't trample the dst_buff, and we treat it as if
		 * dst_len == src_len.
		 */
		status = msgl->write(src_mem, dst_off, src_len);
		assert_fatal(status >= 0);
	}
	else
	{
		printf(WARNING LZUDI"BUF_ALLOC: Unknown permutation of args. "
			"Unsure how to copy. Silently ignoring.\n");
	}

	callback(gcb, msgl);
}

}
