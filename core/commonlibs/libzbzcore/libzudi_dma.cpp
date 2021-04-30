
#include <__kstdlib/__ktypes.h>
#include <__kstdlib/__kcxxlib/new>
#include <kernel/common/process.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/floodplainn/dma.h>
#include <commonlibs/libzbzcore/libzudi.h>


/**	EXPLANATION:
 * The UDI DMA abstractions are built on top of the UDI buf abstractions,
 * and to the extent that this is true, the UDI DMA abstractions mostly consist
 * of userspace constructs.
 *
 * The udi_dma_handle_t is really a handle to a compiled constraints object.
 * The developer can then bind any number of different buffers to that compiled
 * constraints object by using dma_buf_map/unmap.
 *
 * If the buf_path for the given buffer doesn't match the constraints on the
 * udi_dma_handle_t, the environment is *expected* to allocate a new SGList on
 * the fly and then copy the data from the given buffer into that transitory
 * SGList and return the frame-list for the transitory SGList as the
 * udi_scgth_t.
 *
 * This on the fly SGList allocation and (moreso than that) the data copying
 * into the transitory SGList is a bit too cumbersome for me to implement when
 * it can be designed around by the developer. So I will postpone implementation
 * of that portion of the UDI spec.
 *
 * What we currently do instead, is require the developer to make sure that
 * his/her udi_buf_t objects are properly bound to a udi_buf_path_t.
 *
 * This is a way to ensure that the on-the-fly bounce buffer SGList allocation
 * is not needed.
 **/

/**	FIXME: XXX:
 * Big FIXME for this whole file: the `buf_size` member of udi_buf_t is not
 * in any way respected by this file. It really should be.
 */

static sbit8 udi_scgth_t_is_initialized(udi_scgth_t *us)
{
	if (us->scgth_format == 0 || us->scgth_num_elements < 1) {
		return 0;
	}

	return FLAG_TEST(us->scgth_format, UDI_SCGTH_32)
		? (us->scgth_elements.el32p != NULL)
		: (us->scgth_elements.el64p != NULL);
}

lzudi::dma::sHandle *
lzudi::dma::udi_dma_prepare_alloc_and_compile_dmah_sync(
	udi_dma_constraints_t constraints,
	udi_ubit8_t flags
	)
{
	lzudi::dma::sHandle			*ret;
	fplainn::dma::constraints::Compiler	compiledCons;
	fplainn::dma::Constraints		conParser;
	error_t					err;

	if (constraints == NULL || constraints->attrs == NULL) { return NULL; }

	/**	EXPLANATION:
	 * We want to sanity check the constraints given by userspace then
	 * compile them and see if they compile correctly.
	 *
	 * If they compile, we save the compiled constraints into the resultant
	 * udi_dma_handle_t object.
	 *
	 * Later on when a buffer is bound to the udi_dma_handle_t, we will
	 * check on point of call (to udi_dma_buf_map) whether or not the
	 * compiled constraints within the udi_dma_handle_t match those which
	 * were bound to the udi_buf_t.
	 *
	 * But we can't check that now, so just verify that the constraints
	 * passed to *this* function can be compiled at all.
	 */
	ret = new lzudi::dma::sHandle;
	if (ret == NULL)
	{
		printf(ERROR"Failed to alloc metadata for udi_dma_prepare.\n");
		return NULL;
	}

	memset(ret, 0, sizeof(*ret));

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

	ret->compiledConstraints = compiledCons;
	ret->udi_dmah_flags = flags;
	return ret;

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

	ret = lzudi::dma::udi_dma_prepare_alloc_and_compile_dmah_sync(
		constraints, flags);

	if (ret == NULL)
	{
		printf(ERROR LZUDI"dma_prep: failed to alloc or compile "
			"udi_dma_handle_t\n");

		callback(gcb, NULL);
	}

	// Downcast ret.
	callback(gcb, ret);
}

udi_buf_t *udi_dma_buf_unmap(
	udi_dma_handle_t dma_handle,
	udi_size_t new_buf_size
	)
{
	lzudi::buf::MappedScatterGatherList	*msgl;
	lzudi::dma::sHandle			*dmah
		= static_cast<lzudi::dma::sHandle *>(dma_handle);

	(void)new_buf_size;

	if (dma_handle == NULL) {
		return NULL;
	}

	msgl = dmah->msgl;

	if (dmah->msgl == NULL) {
		// If the dmah has no bound buf, we must return early.
		return NULL;
	}

	if (FLAG_TEST(dmah->udiScgthObj.scgth_format, UDI_SCGTH_32)) {
		delete[] dmah->udiScgthObj.scgth_elements.el32p;
	} else {
		delete[] dmah->udiScgthObj.scgth_elements.el64p;
	}

	if (FLAG_TEST(dmah->udi_dmah_flags, UDI_DMA_IN))
	{
		// FIXME: Call sync() here when it's been implemented.
		// udi_dma_sync();

		if (dmah->bounceBuffer != NULL)
		{
			// Copy the data from the bounce buffer
			// msgl->write(dmah->bounceBuffer->read());
			// Destroy the bounce buffer.
			// free_bounce_buffer(dmah->bounceBuffer);
			dmah->bounceBuffer = NULL;
		}
	}

	memset(&dmah->udiScgthObj, 0, sizeof(dmah->udiScgthObj));
	dmah->msgl = NULL;
	return msgl;
}

void udi_dma_free(udi_dma_handle_t dma_handle)
{
	lzudi::dma::sHandle		*dmah;

	/**	EXPLANATION:
	 * If there is no udi_buf_t bound to this constraints object, then just
	 * free the heap object that it points to.
	 */
	if (dma_handle == NULL) { return; }

	udi_dma_buf_unmap(dma_handle, 0);

	// Upcast.
	dmah = static_cast<lzudi::dma::sHandle *>(dma_handle);
	dmah->compiledConstraints.setInvalid();
	dmah->udi_dmah_flags = 0;

	delete dmah;
}

struct sUdi_dma_mem_alloc_args
{
	sUdi_dma_mem_alloc_args(
		udi_dma_mem_alloc_call_t *_callback,
		udi_cb_t *_gcb,
		udi_dma_constraints_t _constraints,
		udi_ubit8_t _flags,
		udi_ubit16_t _nelements,
		udi_size_t _element_size,
		udi_size_t _max_gap
		)
	:
	callback(_callback), gcb(_gcb),
	constraints(_constraints), flags(_flags), nelements(_nelements),
	element_size(_element_size), max_gap(_max_gap)
	{}

	udi_dma_mem_alloc_call_t *callback;
	udi_cb_t *gcb;
	udi_dma_constraints_t constraints;
	udi_ubit8_t flags;
	udi_ubit16_t nelements;
	udi_size_t element_size;
	udi_size_t max_gap;

	struct
	{
		udi_dma_handle_t	dma_handle;
		udi_buf_t		*buf;
	} ret;
};

static udi_dma_prepare_call_t udi_dma_mem_alloc_2;
static udi_buf_write_call_t udi_dma_mem_alloc_3;
static udi_buf_write_call_t udi_dma_mem_alloc_3_5;
static udi_dma_buf_map_call_t udi_dma_mem_alloc_4;

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
	LZUDI_CHECK_GCB_AND_CALLBACK_VALID(
		gcb, callback,
		gcb, NULL, NULL, 0, 0, NULL, 0);

	if (nelements == 0 || element_size == 0) {
		callback(gcb, NULL, NULL, 0, 0, NULL, 0);
	}

	/* We know that this path is synchronous so we don't need to
	 * udi_cb_alloc() since we know the stack will not go out of scope
	 */
	udi_cb_t 			prepcb;
	sUdi_dma_mem_alloc_args		args(
		callback, gcb, constraints, flags,
		nelements, element_size, max_gap);

	memcpy(&prepcb, gcb, sizeof(*gcb));
	prepcb.initiator_context = &args;

	udi_dma_prepare(udi_dma_mem_alloc_2, &prepcb, constraints, flags);
}

static void udi_dma_mem_alloc_2(udi_cb_t *cb, udi_dma_handle_t dma_handle)
{
	sUdi_dma_mem_alloc_args 	*args =
		static_cast<sUdi_dma_mem_alloc_args *>(cb->initiator_context);

	if (dma_handle == NULL)
	{
		args->callback(args->gcb, NULL, NULL, 0, 1, NULL, 0);
		return;
	}

	args->ret.dma_handle = dma_handle;

	UDI_BUF_ALLOC(udi_dma_mem_alloc_3, cb, NULL,
		/* Initialize with size of 0 because we need to constrain the
		 * list before we begin adding frames to it.
		 */
		0,
		0);
}

static void udi_dma_mem_alloc_3(udi_cb_t *gcb, udi_buf_t *new_buf)
{
	sUdi_dma_mem_alloc_args 		*args =
		static_cast<sUdi_dma_mem_alloc_args *>(gcb->initiator_context);
	lzudi::buf::MappedScatterGatherList	*msgl;
	lzudi::dma::sHandle			*dmah;
	Thread					*currThread;
	error_t					err;

	if (new_buf == NULL)
	{
		udi_dma_free(args->ret.dma_handle);
		args->callback(args->gcb, NULL, NULL, 0, 1, NULL, 0);
		return;
	}

	msgl = static_cast<lzudi::buf::MappedScatterGatherList *>(new_buf);
	dmah = static_cast<lzudi::dma::sHandle *>(args->ret.dma_handle);
	currThread = cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentThread();

	/* Constrain the buf by the same constraints that are contained in the
	 * udi_dma_handle_t instance that was allocated earlier on in this
	 * call chain.
	 */
	err = currThread->parent->floodplainnStream.constrainScatterGatherList(
		msgl->sGListIndex, &dmah->compiledConstraints);

	if (err != ERROR_SUCCESS)
	{
		printf(ERROR LZUDI"udi_dma_mem_alloc(): Failed to constrain "
			"internal buf's sglist by dma_handle's constraints.\n");

		udi_dma_free(args->ret.dma_handle);
		udi_buf_free(new_buf);
		args->callback(args->gcb, NULL, NULL, 0, 1, NULL, 0);
		return;
	}

	args->ret.buf = new_buf;

	UDI_BUF_INSERT(
		udi_dma_mem_alloc_3_5, gcb,
		NULL, args->nelements * args->element_size,
		new_buf, 0);
}


static void udi_dma_mem_alloc_3_5(udi_cb_t *gcb, udi_buf_t *new_buf)
{
	sUdi_dma_mem_alloc_args 		*args =
		static_cast<sUdi_dma_mem_alloc_args *>(gcb->initiator_context);

	if (new_buf != NULL) {
		args->ret.buf = new_buf;
	}
	else
	{
		printf(WARNING LZUDI"udi_dma_mem_alloc: Failed to resize "
			"internal buf's sglist after constraining it.\n");
	}

	udi_dma_buf_map(
		udi_dma_mem_alloc_4, gcb,
		args->ret.dma_handle, args->ret.buf,
		0, args->nelements * args->element_size,
		args->flags);
}

static void udi_dma_mem_alloc_4(
	udi_cb_t *gcb,
	udi_scgth_t *scgth,
	udi_boolean_t complete,
	udi_status_t status
	)
{
	sUdi_dma_mem_alloc_args 		*args =
		static_cast<sUdi_dma_mem_alloc_args *>(gcb->initiator_context);
	lzudi::buf::MappedScatterGatherList	*msgl;

	if (status != UDI_OK || !complete || scgth == NULL
		|| !udi_scgth_t_is_initialized(scgth))
	{
		if (!complete)
		{
			printf(ERROR LZUDI"dma_mem_alloc_4: dma_buf_map did "
				"map the whole sglist in one go, as is needed "
				"for dma_mem_alloc.\n");
		}

		if (scgth == NULL)
		{
			printf(ERROR LZUDI"dma_mem_alloc_4: scgth returned "
				"from dma_buf_map is NULL.\n");
		}

		if (!udi_scgth_t_is_initialized(scgth))
		{
			printf(ERROR LZUDI"dma_mem_alloc_4: scgth returned "
				"from dma_buf_map has invalid values/is "
				"uninitialized\n");
		}

		udi_buf_free(args->ret.buf);
		udi_dma_free(args->ret.dma_handle);
		args->callback(args->gcb, NULL, NULL, 0, 1, NULL, 0);
		return;
	}

	// Upcast.
	msgl = static_cast<lzudi::buf::MappedScatterGatherList *>(
		args->ret.buf);

	// FIXME: must_swap needs proper handling.
	args->callback(
		args->gcb, args->ret.dma_handle,
		msgl->getAddrForOffset(0), 0, 0, scgth, 0);
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
	(void)dma_handle;
	(void)src_off;
	(void)src_len;
	(void)dst_buf;

	printf(FATAL LZUDI"dma_mem_to_buf: Unimplemented!\n");
	callback(gcb, NULL);
}

static error_t udi_scgth_t_alloc_and_construct(
	lzudi::buf::MappedScatterGatherList *msgl,
	fplainn::dma::constraints::Compiler *comcons,
	udi_scgth_t *retobj
	)
{
	error_t			ret;
	status_t		stat;
	udi_scgth_element_64_t	*outarr;
	ubit8			inoutarrFmt=0, outarrFmtSupported=0;
	Thread			*currThread;

	currThread = cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentThread();

	if (!comcons->isValid())
	{
		/* If the list was unconstrained, then that's too bad -- the
		 * caller should make sure to constrain the list. In that case
		 * we will make an arbitrary list format choice on behalf of
		 * the caller and if they don't like it, then they will just
		 * have to abort and exit() their driver because they can't
		 * understand the format of the output frame list.
		 *
		 * Alternatively, they could constrain their sglists and avoid
		 * this problem.
		 *
		 * The logic for the default choice we make is:
		 *	* If 64 bit arch, then drivers can natively manipulate
		 * 	  64-bit addresses, hence they are likely to support
		 * 	  them so use 64-bit formatted lists.
		 *	* For 32-bit arch with PAE enabled, we assume that
		 * 	  there is a possibility that the frames in the SGlist
		 * 	  could be above the 4GiB mark, so we are forced to
		 *	  use a 64-bit formatted list -- if the driver doesn't
		 * 	  support them, then too bad.
		 *	* For a 32-bit arch with no PAE, we use a 32-bit
		 *	  formatted list.
		 */
#if __VADDR_NBITS__ == 64 || (__VADDR_NBITS__ == 32 && defined(CONFIG_ARCH_x86_32_PAE))
		outarrFmtSupported = UDI_SCGTH_64 | UDI_SCGTH_32;
#elif __VADDR_NBITS__ == 32
		outarrFmtSupported = UDI_SCGTH_32;
#else

	#error "Unable to make an arbitrary decision for Scgth list output formats"
#endif
	}
	else
	{
		/* We assume that the UDI_DMA_FORMAT flags used to constrain the
		 * list are acceptable to the caller.
		 */
		outarrFmtSupported = comcons->i.callerSupportedFormats;
	}

	inoutarrFmt = outarrFmtSupported;

	/* Allocate enough room for the list we expect to get back. Assume
	 * that it's possible that none of the frame is contiguous and that
	 * none of them could be coalesced.
	 */
	outarr = new udi_scgth_element_64_t[msgl->nFrames];
	if (outarr == NULL) {
		return ERROR_MEMORY_NOMEM;
	}

	// Ask the kernel to give us the list of frames in the SGList.
	stat = currThread->parent->floodplainnStream
		.getScatterGatherListElements(
			msgl->sGListIndex,
			outarr, sizeof(*outarr) * msgl->nFrames,
			&inoutarrFmt);

	if (stat < 0)
	{
		printf(ERROR LZUDI"dma_buf_map: Failed to retrieve frames in "
			"sGList %d.\n",
			msgl->sGListIndex);

		ret = stat;
		goto out_free_scgth_elements;
	}

	/* If the kernel returned list elements of an unsupported type,
	 * return error.
	 */
	if ((inoutarrFmt & outarrFmtSupported) == 0)
	{
		printf(WARNING LZUDI"udi_scgth_t_alloc: list returned by "
			"kernel is of unsupported data type.\n"
			"Requested formats: %x. Returned format: %x.\n",
			outarrFmtSupported, inoutarrFmt);

		ret = ERROR_UNSUPPORTED;
		goto out_free_scgth_elements;
	}

	/* We can return the frame list as a plain array and ignore parameters
	 * like MAX_EL_PER_SEGMENT and MAX_SEGMENTS because we do not support
	 * UDI_SCGTH_DMA_MAPPED.
	 */

	// We do not currently support UDI_SCGTH_DMA_MAPPED.
	retobj->scgth_format = inoutarrFmt | UDI_SCGTH_DRIVER_MAPPED;
	retobj->scgth_must_swap = 0;
	retobj->scgth_num_elements = stat;
	if (FLAG_TEST(inoutarrFmt, UDI_SCGTH_32)) {
		retobj->scgth_elements.el32p = (udi_scgth_element_32_t *)outarr;
	} else {
		retobj->scgth_elements.el64p = outarr;
	}

	return ERROR_SUCCESS;

out_free_scgth_elements:
	delete[] outarr;
	return ret;
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
	lzudi::dma::sHandle	*dmah = static_cast<lzudi::dma::sHandle *>(
		dma_handle);
	lzudi::buf::MappedScatterGatherList		*msgl =
		static_cast<lzudi::buf::MappedScatterGatherList *>(buf);

	fplainn::dma::constraints::Compiler		bufSglCompiledCons;
	Thread						*currThread;
	error_t						err;

	(void)offset;
	(void)len;

	LZUDI_CHECK_GCB_AND_CALLBACK_VALID(
		gcb, callback,
		gcb, NULL, 0, UDI_STAT_NOT_UNDERSTOOD);

	/**	EXPLANATION:
	 * Retrieve the constraints object which has been bound to `buf`.
	 * If there is no constraints object bound to it, retrieve the chipset's
	 * default constraints.
	 * Compare the retrieved constraints against the constraints inside the
	 * udi_dma_handle_t.
	 *
	 * If they don't match, error and bail because we don't support bounce
	 * transitory SGLists (DMA bounce buffers) as yet.
	 *
	 * If they do match, either call FPStream::unmapScatterGatherList or
	 * do nothing because we don't *strictly* need to unmap the SGList
	 * before querying its internal frame list. (It might however be a good
	 * idea to implement a "freeze" method on SGLists to ensure that they
	 * cannot be updated in the kernel until the DMA transaction is
	 * complete).
	 *
	 * At this point, query the internal frame list backing the SGList and
	 * then return that to userspace as a udi_scgth_t.
	 **/
	if (dma_handle == NULL || buf == NULL) {
		(*callback)(gcb, NULL, 0, UDI_STAT_NOT_SUPPORTED);
	}

	if ((dmah->udi_dmah_flags & flags) == 0)
	{
		(*callback)(gcb, NULL, 0, UDI_STAT_INVALID_STATE);
		return;
	}

	/* If the udi_dma_handle_t is already occupied by another memory region,
	 * return BUSY -- the caller should first unmap using
	 * udi_dma_buf_unmap().
	 */
	if (dmah->msgl != NULL
		|| udi_scgth_t_is_initialized(&dmah->udiScgthObj))
	{
		// If one is initialized, the other should be as well.
		assert_fatal(dmah->msgl != NULL);
		assert_fatal(udi_scgth_t_is_initialized(&dmah->udiScgthObj));

		printf(ERROR LZUDI"dma_buf_map: dma_handle_t obj @%p is "
			"already in use. It currently maps SGList %d.\n",
			dmah, dmah->msgl->sGListIndex);

		(*callback)(gcb, NULL, 0, UDI_STAT_BUSY);
		return;
	}

	currThread = cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentThread();

	err = currThread->parent->floodplainnStream
		.getScatterGatherListConstraints(
			msgl->sGListIndex, &bufSglCompiledCons);

	if (err != ERROR_SUCCESS)
	{
		printf(NOTICE LZUDI"dma_buf_map: kSGList %d unconstrained. "
			"Using chipset default constraints.\n");

		err = currThread->parent->floodplainnStream
				.getChipsetDefaultConstraints(
					&bufSglCompiledCons);

		if (err != ERROR_SUCCESS)
		{
			(*callback)(gcb, NULL, 0, UDI_STAT_INVALID_STATE);
			return;
		}
	}

	if (memcmp(
		&bufSglCompiledCons, &dmah->compiledConstraints,
		sizeof(bufSglCompiledCons)) != 0)
	{
		// Alloc a new MappedScatterGatherList as a bounce buffer.
		// dmah->bounceBuffer = alloc_new_bounce_buffer();

		printf(NOTICE LZUDI"dma_buf_map: buf constraints don't match "
			"dma_handle constraints. Aborting.\n"
			"\tIn the future when bounce buffers are supported "
			"this will be where one will be created.\n");

		(*callback)(gcb, NULL, 0, UDI_STAT_ATTR_MISMATCH);
		return;
	}

	// Malloc the memory for the output list.
	err = udi_scgth_t_alloc_and_construct(
		((dmah->bounceBuffer == NULL)
			? msgl
			: dmah->bounceBuffer->msgl),
		&bufSglCompiledCons, &dmah->udiScgthObj);

	if (err != ERROR_SUCCESS)
	{
		printf(ERROR LZUDI"dma_buf_map: Failed to retrieve frames in "
			"%sbounce buffer sGList %d.\n",
			((dmah->bounceBuffer == NULL) ? "NON-" : ""),
			msgl->sGListIndex);

		(*callback)(gcb, NULL, 0, UDI_STAT_NOT_RESPONDING);
		return;
	}

	// Set the dmah to point to the msgl.
	dmah->msgl = msgl;

	if (FLAG_TEST(dmah->udi_dmah_flags, UDI_DMA_OUT))
	{
		if (dmah->bounceBuffer != NULL)
		{
			// copy the data into the bounce buffer.
			// dmah->bounceBuffer->write(msgl->read());
		}

		/* Call udi_dma_sync() to clean the data to cache before the
		 * device attempts to read it.
		 */
		// udi_dma_sync();
	}

	// We're done.
	(*callback)(gcb, &dmah->udiScgthObj, TRUE, UDI_OK);
}

void udi_dma_mem_barrier(udi_dma_handle_t dma_handle)
{
	(void)dma_handle;

	asm volatile("mfence\n\t");
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
	lzudi::dma::sHandle			*dmah;
	lzudi::buf::MappedScatterGatherList	*chosen_msgl;
	ubit8					*cleanStartVaddr;

	LZUDI_CHECK_GCB_AND_CALLBACK_VALID(
		gcb, callback,
		gcb);

	if (dma_handle == NULL) { callback(gcb); return; }
	if (FLAG_TEST(flags, UDI_DMA_IN | UDI_DMA_OUT) == 0)
	{
		callback(gcb);
		return;
	}

	// Upcast.
	dmah = static_cast<lzudi::dma::sHandle *>(dma_handle);

	// udi_dma_sync expects an already-mapped dmah.
	if (dmah->msgl == NULL
		|| !udi_scgth_t_is_initialized(&dmah->udiScgthObj))
	{
		// If one is uninitialized, the other should be as well.
		assert_fatal(dmah->msgl == NULL);
		assert_fatal(!udi_scgth_t_is_initialized(&dmah->udiScgthObj));

		printf(ERROR LZUDI"dma_sync: dma_handle_t obj @%p is "
			"uninitialized.\n",
			dmah);

		callback(gcb);
		return;
	}

	/* I'm not yet sure whether we should perform the cache operation on
	 * the user-supplied buffer or the bounce buffer, if a bounce buffer
	 * is being used. Or, perhaps both need to have the cache operation
	 * applied to them.
	 */
	if (dmah->bounceBuffer)
	{
		assert_fatal(dmah->bounceBuffer->msgl != NULL);
		chosen_msgl = dmah->bounceBuffer->msgl;
	}
	else {
		chosen_msgl = dmah->msgl;
	}

	if (length == 0 && offset != 0)
	{
		printf(WARNING LZUDI"dma_sync: length is 0, but offset "
			"is not 0 (offset is %x).\n"
			"\tThis is non-conformant. Silently assuming "
			"offset=0.\n",
			offset);

		offset = 0;
		length = PAGING_PAGES_TO_BYTES(chosen_msgl->nFrames);
	}
	else
	{
		if (length > PAGING_PAGES_TO_BYTES(chosen_msgl->nFrames))
		{
			printf(ERROR LZUDI"dma_sync: length %d exceeds size of "
				"SGList.\n",
				length);

			callback(gcb);
			return;
		}
	}

	cleanStartVaddr = static_cast<ubit8 *>(
		chosen_msgl->getAddrForOffset(offset));

	if (cleanStartVaddr == NULL)
	{
		printf(ERROR LZUDI"dma_sync: start offset %d within SGList "
			"is invalid.\n",
			offset);

		callback(gcb);
		return;
	}

	asm volatile ("wbinvd\n\t");

	callback(gcb);
}
