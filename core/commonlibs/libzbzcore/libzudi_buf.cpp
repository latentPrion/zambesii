
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

static error_t allocSGListAndConstrainByPathHandle(
	udi_buf_path_t path_handle,
	lzudi::buf::MappedScatterGatherList **retmsgl
	)
{
	error_t						ret;
	Thread						*currThread;
	fplainn::dma::constraints::Compiler		requestedConstraints;

	currThread = cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentThread();

	// path_handle must be supplied when allocating a buf.
	if (UDI_HANDLE_IS_NULL(path_handle, udi_buf_path_t))
	{
		printf(ERROR LZUDI"BUF_ALLOC: A path handle must be "
			"supplied to enable constraint.\n");

		return ERROR_INVALID_FORMAT;
	}

	/**	FIXME:
	 * Ask the kernel for the constraints associated with the
	 * path handle.
	 **/
	ret = currThread->parent->floodplainnStream.
		getParentConstraints(
			reinterpret_cast<uintptr_t>(path_handle),
			&requestedConstraints);

	if (ret != ERROR_SUCCESS)
	{
		printf(ERROR LZUDI"BUF_ALLOC: Failed to get "
			"constraints for new buffer.\n");

		return ret;
	}

	// Allocate a new SGList.
	ret = lzudi::buf::allocateScatterGatherList(
		&requestedConstraints, retmsgl);

	if (ret != ERROR_SUCCESS)
	{
		printf(ERROR LZUDI"BUF_ALLOC: Failed to malloc "
			"handle.\n");

		return ret;
	}

	return ERROR_SUCCESS;
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
	uarch_t						extraNBytesRequired;
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
		if (dst_len != 0)
		{
			printf(WARNING LZUDI"When dst_buf is NULL, dst_len "
				"should be 0 for UDI compliance. Ignoring.\n");
		}

		// dst_buf == NULL means we need to allocate a new scgth list.
		err = allocSGListAndConstrainByPathHandle(
			path_handle, &msgl);

		if (err != ERROR_SUCCESS)
		{
			printf(ERROR LZUDI"BUF_ALLOC: Failed to alloc and "
				"constrain SGList.\n");

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

	/* We don't need to bounds check dst_off + [dst|src]_len because we
	 * ensure that there is enough memory up above using
	 * hasEnoughMemoryForWrite.
	 **/

	if (dst_buf != NULL && dst_len == 0)
	{
		// Handle the insertion case: move data up src_len bytes.
		msgl->memmove(
			dst_off + src_len, dst_off,
			PAGING_PAGES_TO_BYTES(msgl->nFrames)
				- (dst_off + dst_len));
	}

	if (src_mem == NULL && src_len == 0)
	{
		// Deletion case is where src_mem AND src_len are NULL.
		msgl->memmove(
			dst_off, dst_off + dst_len,
			PAGING_PAGES_TO_BYTES(msgl->nFrames)
				- (dst_off + dst_len));
	}
	else if (src_mem == NULL || src_len == 0) {
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
			"Unsure how to copy. Ignoring.\n");
	}

	// There should be a trivial conversion here to udi_buf_t *.
	callback(gcb, msgl);
}

void udi_buf_read(
	udi_buf_t *src_buf,
	udi_size_t src_off,
	udi_size_t src_len,
	void *dst_mem
	)
{
	sarch_t					status;
	lzudi::buf::MappedScatterGatherList	*msgl;

	/*	TODO:
	 * Don't forget to update src_buf->buf_size.
	 */

	assert_fatal(dst_mem != NULL);
	assert_fatal(src_buf != NULL);
	msgl = static_cast<lzudi::buf::MappedScatterGatherList *>(src_buf);

	/* No need for any eccentric case handling since the spec demands that:
	 *	"src_off+src_len must not exceed src_buf->buf_size."
	 */
	status = msgl->read(dst_mem, src_off, src_len);
	assert_fatal(status >= 0);
}

void udi_buf_copy(
	udi_buf_copy_call_t *callback,
	udi_cb_t *gcb,
	udi_buf_t *src_buf,
	udi_size_t src_off,
	udi_size_t src_len,
	udi_buf_t *dst_buf,
	udi_size_t dst_off,
	udi_size_t dst_len,
	udi_buf_path_t path_handle
	)
{
	error_t						err;
	lzudi::buf::MappedScatterGatherList		*dst_msgl, *src_msgl;
	Thread						*currThread;
	uarch_t						extraNBytesRequired;
	void						*src_mem;

	/**	CAVEAT:
	 * This function is not yet UDI fully compliant. There are scenarios
	 * which should be triggered by different combinations of arguments
	 * which are not handled by this code.
	 */

	LZUDI_CHECK_GCB_AND_CALLBACK_VALID(gcb, callback, gcb, NULL);

	/* UDI spec:
	 *	"src_buf is a pointer to the buffer containing data to
	 * 	be copied. This must not be set to NULL."
	 *
	 *	"src_len is the number of bytes to be copied from the source
	 * 	buffer. src_len must be at least 1, and src_off + src_len must
	 * 	not extend beyond the current buffer size"
	 *
	 * I.e, src_buf can't be NULL and src_len can't be 0.
	 **/
	if (src_buf == NULL || src_len < 1)
	{
		printf(ERROR LZUDI"BUF_COPY: src_buf is %p, src_len is %u.\n",
			src_buf, src_len);

		callback(gcb, NULL);
	}

	currThread = cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentThread();

	src_msgl = static_cast<lzudi::buf::MappedScatterGatherList *>(src_buf);

	if (dst_buf == NULL)
	{
		if (dst_len != 0)
		{
			printf(WARNING LZUDI"When dst_buf is NULL, dst_len "
				"should be 0 for UDI compliance. Ignoring.\n");
		}

		// dst_buf == NULL means we have to alloc a new scgth list.
		err = allocSGListAndConstrainByPathHandle(
			path_handle, &dst_msgl);

		if (err != ERROR_SUCCESS)
		{
			callback(gcb, NULL);
			return;
		}

		dst_buf = static_cast<udi_buf_t *>(dst_msgl);
	}

	dst_msgl = static_cast<lzudi::buf::MappedScatterGatherList *>(dst_buf);

	// Make sure the dest buf has enough room.
	if (!dst_msgl->hasEnoughMemoryForWrite(
		currThread, dst_off, dst_len, src_len, &extraNBytesRequired))
	{
		// Attempt to resize the SGList.
		err = currThread->parent->floodplainnStream
			.resizeScatterGatherList(
				dst_msgl->sGListIndex,
				dst_msgl->nFrames + PAGING_BYTES_TO_PAGES(
					extraNBytesRequired));

		if (err != ERROR_SUCCESS)
		{
			printf(ERROR LZUDI"BUF_COPY: Failed to resize buf.\n");
			callback(gcb, NULL);
			return;
		}

		if (!dst_msgl->hasEnoughMemoryForWrite(
			currThread,
			dst_off, dst_len, src_len, &extraNBytesRequired))
		{
			printf(ERROR LZUDI"BUF_COPY: Resize returned success, "
				"yet still not enough memory for write.\n");

			callback(gcb, NULL);
			return;
		}

		err = currThread->parent->floodplainnStream
			.remapScatterGatherList(
				dst_msgl->sGListIndex, &dst_msgl->vaddr);

		if (err != ERROR_SUCCESS)
		{
			printf(ERROR LZUDI"BUF_COPY: Remap after resize "
				"failed!\n");

			callback(gcb, NULL);
			return;
		}
	}

	/* "src_off is the offset, in bytes, from the first logical data byte
	 * to the start of the copy area in the source buffer. This must not
	 * exceed the current size of the buffer:
	 *	0 <= src_off < src_buf->buf_size
	 *
	 * src_off gets checked in getAddrForOffset below.
	 **/

	// Perform the copy.
	src_mem = src_msgl->getAddrForOffset(src_off);
	if (src_mem == NULL)
	{
		printf(ERROR LZUDI"BUF_COPY: src buf offset %d invalid!\n",
			src_off);

		callback(gcb, NULL);
		return;
	}

	/* From here there are 2 basic use cases for udi_buf_copy:
	 * overwrite and insert.
	 *
	 * When dst_len is 0 then we are inserting;
	 * Else we are overwriting.
	 */

	if (dst_len == 0)
	{
		// Inserting use case: move the current data aside first.
		dst_msgl->memmove(dst_off + src_len, dst_off, src_len);
	}

	dst_msgl->write(src_mem, dst_off, src_len);
	// There should be a trivial conversion here to udi_buf_t *.
	callback(gcb, dst_msgl);
}

void udi_buf_free(udi_buf_t *buf)
{
	lzudi::buf::MappedScatterGatherList	*msgl;
	Thread					*currThread;
	sbit8					succeeded;

	if (buf == NULL) {
		return;
	}

	msgl = static_cast<lzudi::buf::MappedScatterGatherList *>(buf);
	currThread = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();

	succeeded = currThread->parent->floodplainnStream
		.releaseScatterGatherList(msgl->sGListIndex, 0);

	if (!succeeded)
	{
		printf(FATAL LZUDI"buf_free: FPStream:releaseSGList(%d) failed."
			"\n", msgl->sGListIndex);
	}

	delete msgl;
}
