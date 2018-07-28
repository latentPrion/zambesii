
#include <__kstdlib/__ktypes.h>
#include <__kstdlib/__kcxxlib/new>
#include <kernel/common/process.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
//#include <kernel/common/floodplainn/dma.h>
#include <commonlibs/libzbzcore/libzudi.h>


lzudi::dma::sHandle *
lzudi::dma::udi_dma_prepare_sync(
	udi_dma_constraints_t constraints,
	udi_ubit8_t flags
	)
{
	sarch_t					sGListIndex;
	Thread					*currThread;
	fplainn::dma::ScatterGatherList		*sgl;
	lzudi::dma::sHandle				*ret;
	fplainn::dma::Constraints		conParser;
	fplainn::dma::constraints::Compiler	compiledCons;
	error_t					err;

	if (constraints == NULL || constraints->attrs == NULL) { return NULL; }

	/**	EXPLANATION:
	 * We want to sanity check the constraints given by userspace then
	 * compile them and see if they compile correctly.
	 *
	 * If they do, then we need to call allocateScatterGatherList() and save
	 * the ID. Then constrain the SGList with the compiled constraints.
	 *
	 * If all of that works out, we can return the lzudi::dma::sHandle object
	 * which maintains the context for this SGList.
	 */
	ret = new lzudi::dma::sHandle;
	if (ret == NULL)
	{
		printf(ERROR"Failed to alloc metadata for udi_dma_prepare.\n");
		return NULL;
	}

	memset(ret, 0, sizeof(*ret));

	currThread = cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentThread();

	// Initialize a constraints parser.
	err = conParser.initialize(constraints->attrs, constraints->nAttrs);
	if (err != ERROR_SUCCESS)
	{
		printf(ERROR"Failed to initialize parser for attrs.\n");
		goto out_freeDmaHandle;
	}

	// Initialize a constraints compiler.
	err = compiledCons.initialize();
	if (err != ERROR_SUCCESS)
	{
		printf(ERROR"Failed to constraint compiler for attrs.\n");
		goto out_freeDmaHandle;
	}

	// Compile the constraints.
	err = compiledCons.compile(&conParser);
	if (err != ERROR_SUCCESS)
	{
		printf(ERROR"Failed to compile constraints passed by "
			"userspace.\n");
		goto out_freeDmaHandle;
	}

	ret->sGListIndex = currThread->parent->floodplainnStream
		.allocateScatterGatherList(&ret->sGList);

	if (ret->sGListIndex < 0)
	{
		printf(ERROR"allocSGList() syscall failed. Ret is %d.\n",
			ret->sGListIndex);
		goto out_freeDmaHandle;
	}

	err = ret->sGList->constrain(&compiledCons);
	if (ret != ERROR_SUCCESS)
	{
		printf(ERROR"Failed to constrain newly allocated SGList.\n");
		goto out_releaseSGList;
	}

	return ret;

out_releaseSGList:
	currThread->parent->floodplainnStream.releaseScatterGatherList(
		ret->sGListIndex);

out_freeDmaHandle:
	delete ret;
	return NULL;
}

void udi_dma_prepare(
	udi_dma_prepare_call_t *callback,
	udi_cb_t *gcb,
	udi_dma_constraints_t constraints,
	udi_ubit8_t flags
	)
{
	lzudi::dma::sHandle		*ret;

	LZUDI_CHECK_GCB_AND_CALLBACK_VALID(
		gcb, callback,
		gcb, NULL);

	if (constraints == NULL || constraints->attrs == NULL)
	{
		callback(gcb, NULL);
		return;
	}

	ret = lzudi::dma::udi_dma_prepare_sync(
		constraints, flags);

	callback(gcb, reinterpret_cast<udi_dma_handle_t *>(ret));
}

void udi_dma_free(udi_dma_handle_t dma_handle)
{
}

void udi_dma_mem_alloc(
	udi_dma_mem_alloc_call_t *callback,
	udi_cb_t *gcb,
	udi_dma_constraints_t constraints,
	udi_ubit8_t flags,
	udi_ubit16_t nelements,
	udi_size_t element_size,
	udi_size_t max_gap
	)
{
	lzudi::dma::sHandle	*dmah;

	LZUDI_CHECK_GCB_AND_CALLBACK_VALID(
		gcb, callback,
		gcb, NULL, NULL, 0, 0, NULL, 0);

	if (nelements == 0 || element_size == 0) {
		callback(gcb, NULL, NULL, 0, 0, NULL, 0);
	}

	/**	EXPLANATION:
	 * Call MapScatterGatherList, but don't construct a udi_buf_t on top of
	 * it.
	 **/
	dmah = lzudi::dma::udi_dma_prepare_sync(constraints, flags);
	if (dmah == NULL)
	{
		printf(ERROR"udi_dma_mem_alloc(%d,%d): prepare_sync operation "
			"failed.\n",
			nelements, element_size);

		goto out_freeDmah;
	}

out_freeDmah:
	delete dmah;
	/**	FIXME:
	 * mem_ptr cannot actually return NULL, unlike udi_dma_prepare().
	 * We have to artificially allocate a single element somehow.
	 *
	 * This current behaviour is non-compliant.
	 */
	callback(gcb, NULL, NULL, 0, 0, NULL, 0);
}

void udi_dma_mem_to_buf(
	udi_dma_mem_to_buf_call_t *callback,
	udi_cb_t *gcb,
	udi_dma_handle_t dma_handle,
	udi_size_t src_off,
	udi_size_t src_len,
	udi_buf_t *dst_buf
	)
{
	/**	EXPLANATION:
	 * This is essentially a no-op.
	 **/
}

void udi_dma_buf_map(
	udi_dma_buf_map_call_t *callback,
	udi_cb_t *gcb,
	udi_dma_handle_t dma_handle,
	udi_buf_t *buf,
	udi_size_t offset,
	udi_size_t len,
	udi_ubit8_t flags
	)
{
	lzudi::dma::sHandle	*dmah = reinterpret_cast<lzudi::dma::sHandle *>(
		dma_handle);

	/**	EXPLANATION:
	 * Call udi_dma_mem_alloc(), then call udi_dma_mem_to_buf() on the
	 * memory returned from the former.
	 */
	if (dmah == NULL) { (*callback)(gcb, NULL, 0, UDI_STAT_NOT_SUPPORTED); }
}

void udi_dma_mem_barrier(udi_dma_handle_t dma_handle)
{
}

void udi_dma_sync(
	udi_dma_sync_call_t *callback,
	udi_cb_t *gcb,
	udi_dma_handle_t dma_handle,
	udi_size_t offset,
	udi_size_t length,
	udi_ubit8_t flags
	)
{
}
