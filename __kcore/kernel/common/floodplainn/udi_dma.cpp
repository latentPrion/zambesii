
#include <arch/walkerPageRanger.h>
#include <__kstdlib/__kclib/assert.h>
#include <kernel/common/panic.h>
#include <kernel/common/process.h>
#include <kernel/common/memoryTrib/vaddrSpaceStream.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/floodplainn/dma.h>
#include <kernel/common/floodplainn/zudi.h>

utf8Char *fplainn::dma::Constraints::attrTypeNames[N_ATTR_TYPE_NAMES] = {
	CC"ADDRESSABLE_BITS",
	CC"ALIGNMENT_BITS",

	CC"DATA_ADDRESSABLE_BITS",
	CC"NO_PARTIAL",

	CC"SCGTH_MAX_ELEMENTS",
	CC"SCGTH_FORMAT",
	CC"SCGTH_ENDIANNESS",
	CC"SCGTH_ADDRESSABLE_BITS",
	CC"SCGTH_MAX_SEGMENTS",

	CC"SCGTH_ALIGNMENT_BITS",
	CC"SCGTH_MAX_EL_PER_SEG",
	CC"SCGTH_PREFIX_BYTES",

	CC"ELEMENT_ALIGNMENT_BITS",
	CC"ELEMENT_LENGTH_BITS",
	CC"ELEMENT_GRANULARITY_BITS",

	CC"ADDR_FIXED_BITS",
	CC"ADDR_FIXED_TYPE",
	CC"ADDR_FIXED_VALUE_LO",
	CC"ADDR_FIXED_VALUE_HI",

	CC"SEQUENTIAL",
	CC"SLOP_IN_BITS",
	CC"SLOP_OUT_BITS",
	CC"SLOP_OUT_EXTRA",
	CC"SLOP_BARRIER_BITS",

	CC"LITTLE_ENDIAN",
	CC"BIG_ENDIAN",

	CC"FIXED_ELEMENT",
	CC"FIXED_LIST",
	CC"FIXED_VALUE"
};

static utf8Char		*unknownString = CC"<UNKNOWN>";

sbit8 fplainn::dma::Constraints::isValidConstraintAttrType(
	udi_dma_constraints_attr_t a
	)
{
	if (a < UDI_DMA_ATTR_SPEC_TYPE_MIN || a > UDI_DMA_ATTR_SPEC_TYPE_MAX)
		{ return 0; };

	if (a >= UDI_DMA_SEQUENTIAL && a <= UDI_DMA_SLOP_BARRIER_BITS)
		{ return 1; };

	if (a >= UDI_DMA_ADDR_FIXED_BITS && a <= UDI_DMA_ADDR_FIXED_VALUE_HI)
		{ return 1; };

	if (a >= UDI_DMA_ELEMENT_ALIGNMENT_BITS
		&& a <= UDI_DMA_ELEMENT_GRANULARITY_BITS)
	{
		return 1;
	};

	if (a >= UDI_DMA_SCGTH_ALIGNMENT_BITS && a <= UDI_DMA_SCGTH_PREFIX_BYTES)
		{ return 1; };

	if (a >= UDI_DMA_SCGTH_MAX_ELEMENTS && a <= UDI_DMA_SCGTH_MAX_SEGMENTS)
		{ return 1; };

	if (a >= UDI_DMA_DATA_ADDRESSABLE_BITS && a <= UDI_DMA_NO_PARTIAL)
		{ return 1; };

	if (a >= UDI_DMA_ADDRESSABLE_BITS && a <= UDI_DMA_ALIGNMENT_BITS)
		{ return 1; };

	return 0;
}

void *fplainn::dma::scatterGatherLists::getPages(Thread *t, uarch_t nPages)
{
	return t->parent->getVaddrSpaceStream()->getPages(nPages);
}

void fplainn::dma::scatterGatherLists::releasePages(
	Thread *t, void *vaddr, uarch_t nPages
	)
{
	return t->parent->getVaddrSpaceStream()->releasePages(vaddr, nPages);
}

status_t fplainn::dma::scatterGatherLists::wprUnmap(
	Thread *t, void *v, uarch_t nFrames
	)
{
	paddr_t		p;
	uarch_t		f;

	return walkerPageRanger::unmap(
		&t->parent->getVaddrSpaceStream()->vaddrSpace,
		v, &p, nFrames, &f);
}

status_t fplainn::dma::scatterGatherLists::wprMapInc(
	Thread *t, void *v, paddr_t p, uarch_t nFrames
	)
{
	sbit8			fKernel;

	fKernel = (t->parent->execDomain == PROCESS_EXECDOMAIN_KERNEL)
		? 1 : 0;

	return walkerPageRanger::mapInc(
		&t->parent->getVaddrSpaceStream()->vaddrSpace,
		v, p, nFrames,
		(PAGEATTRIB_PRESENT | PAGEATTRIB_WRITE
		| ((fKernel) ? PAGEATTRIB_SUPERVISOR : 0)));
}

utf8Char *fplainn::dma::Constraints::getAttrTypeName(
	udi_dma_constraints_attr_t a
	)
{
	if (!isValidConstraintAttrType(a))
		{ return unknownString; };

	if (a >= UDI_DMA_SEQUENTIAL)
		{ return attrTypeNames[a - UDI_DMA_SEQUENTIAL + 19]; };

	if (a >= UDI_DMA_ADDR_FIXED_BITS)
		{ return attrTypeNames[a - UDI_DMA_ADDR_FIXED_BITS + 15]; };

	if (a >= UDI_DMA_ELEMENT_ALIGNMENT_BITS) {
		return attrTypeNames[a - UDI_DMA_ELEMENT_ALIGNMENT_BITS + 12];
	};

	if (a >= UDI_DMA_SCGTH_ALIGNMENT_BITS)
		{ return attrTypeNames[a - UDI_DMA_SCGTH_ALIGNMENT_BITS + 9]; };

	if (a >= UDI_DMA_SCGTH_MAX_ELEMENTS)
		{ return attrTypeNames[a - UDI_DMA_SCGTH_MAX_ELEMENTS + 4]; };

	if (a >= UDI_DMA_DATA_ADDRESSABLE_BITS) {
		return attrTypeNames[a - UDI_DMA_DATA_ADDRESSABLE_BITS + 2];
	};

	if (a >= UDI_DMA_ADDRESSABLE_BITS)
		{ return attrTypeNames[a - UDI_DMA_ADDRESSABLE_BITS + 0]; };

	/* This should never be reached. */
	panic(ERROR_UNKNOWN, CC"Fell through to code which should be "
		"unreachable in getAttrTypeName().");
}

void fplainn::dma::Constraints::dump(void)
{
	printf(NOTICE"DMA Constraints obj @%p, %d attrs: dumping.\n",
		this, nAttrs);

	for (uarch_t i=0; i<nAttrs; i++)
	{
		udi_dma_constraints_attr_spec_t		*tmp=&attrs[i];

		printf(CC"\tAttr %s,\t\tValue %x.\n",
			getAttrTypeName(tmp->attr_type),
			tmp->attr_value);
	};
}

error_t fplainn::dma::Constraints::addOrModifyAttrs(
	udi_dma_constraints_attr_spec_t *_attrs, uarch_t _nAttrs
	)
{
	uarch_t			nNewAttrs=0;

	if (_attrs == NULL) { return ERROR_INVALID_ARG; };
	if (_nAttrs == 0) { return ERROR_SUCCESS; };

	// How many of the attrs are new ones?
	for (uarch_t i=0; i<_nAttrs; i++) {
		// Silently ignore invalid attr_type values from userspace.
		if (!isValidConstraintAttrType(_attrs[i].attr_type))
			{ continue; }

		if (!attrAlreadySet(_attrs[i].attr_type)) { nNewAttrs++; };
	};

	assert_fatal(nAttrs + nNewAttrs <= N_ATTR_TYPE_NAMES);

	for (uarch_t i=0, newAttrIndex=0; i<_nAttrs; i++)
	{
		if (!isValidConstraintAttrType(_attrs[i].attr_type))
			{ continue; }

		if (attrAlreadySet(_attrs[i].attr_type))
		{
			udi_dma_constraints_attr_spec_t		*s;

			s = getAttr(_attrs[i].attr_type);
			if (s == NULL)
			{
				printf(FATAL"Attribute %d(%s) disappeared "
					"somehow between the time we examined "
					"it and the time we tried to fetch it.",
					_attrs[i].attr_type);
				panic(ERROR_UNKNOWN);
			}

			s->attr_value = _attrs[i].attr_value;
		}
		else
		{
			assert_fatal(newAttrIndex < nNewAttrs);
			attrs[nAttrs + newAttrIndex] = _attrs[i];
			newAttrIndex++;
		}
	}

	nAttrs += nNewAttrs;
	return ERROR_SUCCESS;
}

error_t fplainn::dma::MappedScatterGatherList::trackPages(
	void *_vaddr, uarch_t _nPages
	)
{
	vaddr = _vaddr;
	nPages = _nPages;
	return ERROR_SUCCESS;
}

void fplainn::dma::ScatterGatherList::dump(void)
{
	printf(NOTICE "ScGthList: %d elements, dumping:\n",
		elements.getNIndexes());

	for (typename SGListElementArray::Iterator it=elements.begin();
		it != elements.end(); ++it)
	{
		__kscgth_element_type_t		tmp = *it;
		paddr_t				p;

		assign_scgth_block_busaddr_to_paddr(
			p, tmp.block_busaddr);

		printf(CC"\tElement: Paddr %P, nBytes %d.\n",
			&p, tmp.block_length);
	}
}

status_t fplainn::dma::ScatterGatherList::addFrames(
	paddr_t p, uarch_t nFrames, sarch_t atIndex, uarch_t flags
	)
{
	error_t			ret;
	sbit8			dontTakeLock;

	/**	EXPLANATION:
	 * Returns ERROR_SUCCESS if there was no need to allocate a new
	 * SGList element.
	 *
	 * If atIndex is non-negative, it is taken to be a placement index in
	 * the array, at which the frames should be added. Whatever frames exist
	 * at that index will be overwritten.
	 *
	 * Returns 1 if a new element was created.
	 *
	 * Returns negative integer value on error.
	 **/

	dontTakeLock = FLAG_TEST(flags, AF_FLAGS_UNLOCKED) ? 1 : 0;

	if (atIndex >= 0) {
		__kscgth_element_type_t		newElement;

		assign_paddr_to_scgth_block_busaddr(&newElement, p);
		newElement.block_length = PAGING_PAGES_TO_BYTES(nFrames);

		if (!dontTakeLock) { lock(); }
		elements[atIndex] = newElement;
		if (!dontTakeLock) { unlock(); }
		return ERROR_SUCCESS;
	}

	if (!dontTakeLock) { lock(); }

	for (
		typename SGListElementArray::Iterator it=
			elements.begin();
		it != elements.end();
		++it)
	{
		// The dereference is unary operator* on class Iterator.
		__kscgth_element_type_t		*tmp=&*it;

		// Can the new paddr be prepended to this element?
		if (::operator==(p + PAGING_PAGES_TO_BYTES(nFrames), *tmp))
		{
			assign_paddr_to_scgth_block_busaddr(tmp, p);
			tmp->block_length += PAGING_PAGES_TO_BYTES(nFrames);

			if (!dontTakeLock) { unlock(); }
			return ERROR_SUCCESS;
		}
		// Can the new paddr be appended to this element?
		else if (::operator==(p - tmp->block_length, *tmp))
		{
			tmp->block_length += PAGING_PAGES_TO_BYTES(nFrames);

			if (!dontTakeLock) { unlock(); }
			return ERROR_SUCCESS;
		};
	};

	/* If we reached here, then we need to add a new element altogether.
	 **/
	uarch_t				prevNIndexes;
	__kscgth_element_type_t		newElement;

	// Resize the list to hold the new SGList element.
	prevNIndexes = elements.unlocked_getNIndexes();
	ret = elements.resizeToHoldIndex(
		prevNIndexes,
		SGListElementArray::RTHI_FLAGS_UNLOCKED);

	if (!dontTakeLock) { unlock(); }

	if (ret != ERROR_SUCCESS) { return ret; };

	// Initialize the new element's values.
	memset(&newElement, 0, sizeof(newElement));
	assign_paddr_to_scgth_block_busaddr(&newElement, p);
	newElement.block_length = PAGING_PAGES_TO_BYTES(nFrames);

	// Finally add it to the list.
	elements[prevNIndexes] = newElement;
	return 1;
}

error_t fplainn::dma::ScatterGatherList::preallocateEntries(
	uarch_t nEntries, uarch_t flags
	)
{
	error_t ret;
	uarch_t fRthi;

	if (nEntries == 0) { return ERROR_SUCCESS; };

	// Only pass on the ResizeableArray flags into resizeToHoldIndex
	fRthi = (FLAG_TEST(flags, PE_FLAGS_UNLOCKED))
		? ResizeableArray<void *>::RTHI_FLAGS_UNLOCKED : 0
	;

	ret = elements.resizeToHoldIndex(
		nEntries - 1, fRthi);

	if (ret != ERROR_SUCCESS) {
		return ret;
	};

	/*	TODO:
	 * Really inefficient? Cycle through and set
	 * the block_length member of each of the new
	 * elements to be 0, indicating that it's an
	 * unused element.
	 */
	return ret;
}

uarch_t fplainn::dma::ScatterGatherList::getNFrames(uarch_t flags)
{
	uarch_t							ret=0;
	typename SGListElementArray::Iterator	it;

	if (!FLAG_TEST(flags, ScatterGatherList::GNF_FLAGS_UNLOCKED))
		{ lock(); }

	for (it = elements.begin(); it != elements.end(); ++it)
	{
		__kscgth_element_type_t		*el = &*it;
		uarch_t				currNFrames;

		currNFrames = PAGING_BYTES_TO_PAGES(el->block_length);
		ret += currNFrames;
	}

	if (!FLAG_TEST(flags, ScatterGatherList::GNF_FLAGS_UNLOCKED))
		{ unlock(); }

	return ret;
}

status_t fplainn::dma::ScatterGatherList::getElements(
	void *outarr, uarch_t nelem, ubit8 *outarrType
	)
{
	class SGListElementArray::Iterator	it;
	uarch_t							i;

	if (outarr == NULL || outarrType == NULL) { return ERROR_INVALID_ARG; }

	if ((*outarrType & (UDI_SCGTH_32 | UDI_SCGTH_64)) == 0) {
		printf(ERROR SGLIST"getElements: Caller must state supported "
			"output list formats. Pass UDI_SCGTH_[32|64].\n");

		return ERROR_INVALID_ARG_VAL;
	}

	/* Prefer to support 64-bit output lists if the caller indicates support
	 * for them.
	 *
	 * But only one sglist format flag must be set by the kernel as the
	 * output `outarrType` returned to the user.
	 */
	if (FLAG_TEST(*outarrType, UDI_SCGTH_64)) {
		*outarrType = UDI_SCGTH_64;
	} else {
		// Nothing to do -- 32 was already set by caller.
	}

	lock();

	// See if the supplied outarr can hold all the elements.
	for (i=0, it=elements.begin();
		i < nelem && it != elements.end();
		++it, i++)
	{
		__kscgth_element_type_t		tmp = *it;
		paddr_t				p;

		assign_scgth_block_busaddr_to_paddr(p, tmp.block_busaddr);

		switch (*outarrType)
		{
		case UDI_SCGTH_32:
			{
			udi_scgth_element_32_t			*outarr32;

			outarr32 = static_cast<udi_scgth_element_32_t *>(outarr);

			outarr32[i].block_length = tmp.block_length;
			assign_paddr_to_scgth_block_busaddr(&outarr32[i], p);
			}
			break;
		default:
			{
			udi_scgth_element_64_t			*outarr64;

			outarr64 = static_cast<udi_scgth_element_64_t *>(outarr);

			outarr64[i].block_length = tmp.block_length;
			assign_paddr_to_scgth_block_busaddr(&outarr64[i], p);
			}
		}
	}

	unlock();
	return i;
}

error_t fplainn::dma::ScatterGatherList::resize(uarch_t nFrames)
{
	sarch_t		currNFrames;
	status_t	nNewFrames;

	/**	EXPLANATION:
	 * Causes this SGList's physical memory to be expanded to at least
	 * nFrames.
	 **/
	if (nFrames == 0) { return ERROR_SUCCESS; }

	lock();
	currNFrames = getNFrames(GNF_FLAGS_UNLOCKED);
	nNewFrames = unlocked_resize(nFrames);
	unlock();

	if ((currNFrames + nNewFrames) < (signed)nFrames)
	{
		// Just to be sure about the behaviour of constrainedGetFrames.
		assert_fatal(nNewFrames < 0);

		printf(ERROR SGLIST"resize(nFrames): Op failed. "
			"PrevNFrames %d, %d allocated newly and will be "
			"freed.\n",
			nFrames, currNFrames, nNewFrames);

		return nNewFrames;
	}

	return ERROR_SUCCESS;
}

error_t fplainn::dma::ScatterGatherList::map(void)
{
	uarch_t			nFrames, nFramesMapped;
	Thread			*currThread;
	typename SGListElementArray::Iterator		it;
	void			*vaddr;
	uintptr_t		currVaddr;
	error_t			ret;
	status_t		status;
	paddr_t			p;

	/*	EXPLANATION:
	 * Map a scgth list into the vaddrspace of the caller. This is fine
	 * because only the caller should have a handle-ID to any sglist.
	 *
	 * MappedSGLists are not demand mapped. They are fully mapped and
	 * committed.
	 */

	currThread = cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentThread();

	lock();

	nFrames = getNFrames(GNF_FLAGS_UNLOCKED);

	vaddr = scatterGatherLists::getPages(currThread, nFrames);
	if (vaddr == NULL)
	{
		unlock();
		printf(ERROR SGLIST"map: Failed to alloc vmem to map the %d "
			"frames in the list.\n",
			nFrames);

		return ERROR_MEMORY_NOMEM_VIRTUAL;
	}

	currVaddr = (uintptr_t)vaddr;
	nFramesMapped = 0;
	for (it = elements.begin(); it != elements.end(); ++it)
	{
		__kscgth_element_type_t			*tmp=&*it;
		uarch_t					nPagesInBlock;

		// Map each paddr + length segment.
		assign_scgth_block_busaddr_to_paddr(p, tmp->block_busaddr);
		nPagesInBlock = PAGING_BYTES_TO_PAGES(tmp->block_length);

		status = scatterGatherLists::wprMapInc(
			currThread, (void *)currVaddr, p, nPagesInBlock);

		if (status < (sarch_t)nPagesInBlock)
		{
			unlock();

			printf(ERROR SGLIST"map: Failed to map all %d pages "
				"for SGList element P%P, %d frames.\n",
				nPagesInBlock, &p);

			ret = ERROR_MEMORY_VIRTUAL_PAGEMAP;
			goto unmapAndReleaseVmem;
		}

		currVaddr += tmp->block_length;
		assert_fatal((currVaddr & PAGING_BASE_MASK_LOW) == 0);
		nFramesMapped += nPagesInBlock;

		// Ensure we don't exceed the amount of loose vmem allocated.
		assert_fatal(nFramesMapped <= nFrames);
	}

	unlock();

	ret = mapping.trackPages(vaddr, nFrames);
	if (ret != ERROR_SUCCESS)
	{
		printf(ERROR SGLIST"map: Failed to track vmem pages at %p in "
			"SGList mapping.\n",
			vaddr);

		// 'ret' was already set.
		goto unmapAndReleaseVmem;
	}

	return ERROR_SUCCESS;

unmapAndReleaseVmem:
	status = scatterGatherLists::wprUnmap(currThread, vaddr, nFrames);
	if (status < (sarch_t)nFrames)
	{
		printf(FATAL SGLIST"map: Failed to clean up. Only %d of %d "
			"pages were unmapped from vaddrspace.\n",
			status, nFrames);

		// Fallthrough.
	}

	scatterGatherLists::releasePages(currThread, vaddr, nFrames);
	return ret;
}

void fplainn::dma::ScatterGatherList::unmap(void)
{
	ProcessStream		*currProcess;
	VaddrSpaceStream	*currVasStream;
	paddr_t			p;
	uarch_t			f;

	if (mapping.vaddr == NULL || mapping.nPages == 0) { return; }

	currProcess = cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentThread()->parent;

	currVasStream = currProcess->getVaddrSpaceStream();

	walkerPageRanger::unmap(
		&currVasStream->vaddrSpace, mapping.vaddr,
		&p, mapping.nPages, &f);

	currVasStream->releasePages(mapping.vaddr, mapping.nPages);
	mapping.trackPages(NULL, 0);
}

status_t fplainn::dma::ScatterGatherList::unlocked_resize(uarch_t nFrames)
{
	uarch_t		currNFrames;

	currNFrames = getNFrames(ScatterGatherList::GNF_FLAGS_UNLOCKED);
	if (currNFrames >= nFrames) { return ERROR_SUCCESS; }

	return memoryTrib.constrainedGetFrames(
		&compiledConstraints,
		// Alloc as many frames as are needed to make up the difference.
		nFrames - currNFrames,
		this,
		MemoryTrib::CGF_SGLIST_UNLOCKED);
}

fplainn::dma::ScatterGatherList::~ScatterGatherList(void)
{
	destroySGList(0);
}

void fplainn::dma::ScatterGatherList::destroySGList(sbit8 justTransfer)
{
	unmap();

	if (!justTransfer) {
		freeSGListElements();
	}

	compiledConstraints.setInvalid();
}

udi_dma_constraints_attr_spec_t *fplainn::dma::Constraints::getAttr(
	udi_dma_constraints_attr_t attr
	)
{
	for (uarch_t i=0; i<nAttrs; i++)
	{
		udi_dma_constraints_attr_spec_t		*tmp=&attrs[i];

		if (tmp->attr_type != attr) { continue; };
		return tmp;
	};

	return NULL;
}

void fplainn::dma::constraints::Compiler::dump(void)
{
	printf(NOTICE"DMACon Compiler @%p. Dumping.\n"
		"\tCan address %u bits\n"
		"\tCan be presented with descriptors of "
		"sizes: 32? %s | 64? %s.\n"
		"\tstarting at PFN %x upto PFN %x.\n"
		"\tSkip stride of %d frames.\n"
		"\tAllocates in blocks of at least %d frames, up to a max of %u frames per elem.\n"
		"\tSlop in is %d bits, out is %d bits.\n"
		"\tPartial alloc? %s. Sequentially accessed? %s.\n",
		this,
		i.addressableBits,
		(FLAG_TEST(i.callerSupportedFormats, UDI_SCGTH_32) ? "yes" : "no"),
		(FLAG_TEST(i.callerSupportedFormats, UDI_SCGTH_64) ? "yes" : "no"),
		i.startPfn, i.beyondEndPfn - 1,
		i.pfnSkipStride,
		i.minElementGranularityNFrames, i.maxNContiguousFrames,
		i.slopInBits, i.slopOutBits,
		((i.partialAllocationsDisallowed) ? "no" : "yes"),
		((i.sequentialAccessHint) ? "yes" : "no"));
}

error_t fplainn::dma::constraints::Compiler::compile(
	fplainn::dma::Constraints *cons
	)
{
	const udi_dma_constraints_attr_spec_t	*tmpAttr;

	/**	EXPLANATION:
	 * We take a constraints specification, and try to allocate "nFrames"
	 * frames worth of RAM in accordance with it.
	 *
	 *	1. UDI_DMA_DATA_ADDRESSABLE_BITS tells us which bit in the BMP
	 *		we have to start at.
	 *	2. UDI_DMA_ADDR_FIXED_BITS further constrains us to some starting
	 *		bit.
	 *	3. UDI_DMA_ELEMENT_ALIGNMENT_BITS tells us our skip-stride when
	 *		allocating.
	 *	4. UDI_DMA_ELEMENT_GRANULARITY_BITS tells us the minimum size
	 *		for each element in the list. We reject any constraints spec
	 *		that speciifies a minimum granularity less than
	 *		PAGING_BASE_SIZE.
	 *	5. UDI_DMA_ELEMENT_LENGTH_BITS tells us the max size each
	 *		element can be.
	 *
	 *	6. UDI_DMA_SCGTH_MAX_EL_PER_SEG tells us when to break into
	 *		a new segment.
	 *	7. UDI_DMA_SCGTH_MAX_ELEMENTS tells us when we should break out
	 *		and fail, telling the caller that we couldn't allocate enough
	 *		frames without violating its MAX_ELEMENTS constraint.
	 *
	 * * We handle SLOP by allocating an extra frame before and after the
	 *   constraint-obeying frames. This opens us up to a DoS attack in
	 *   which the caller can modify his SLOP specification such that we
	 *   aren't aware of the SLOP rounding when freeing a DMA allocation
	 *   that had slop.
	 * * We do not support SLOP greater than PAGING_BASE_SIZE.
	 * * We currently perform no optimizations for the case of
	 *   UDI_DMA_SEQUENTIAL.
	 * * We do not support UDI_SCGTH_DMA_MAPPED.
	 * * We don't support UDI_DMA_FIXED_LIST.
	 *
	 * So what we're trying to do is allocate nFrames, starting at
	 * UDI_DMA_DATA_ADDRESSABLE_BITS + UDI_DMA_ADDR_FIXED_*.
	 *
	 * This function doesn't care when we break into new elements, etc. Just
	 * concerns itself with allocating the frames and adding them to the
	 * retlist.
	 */

	i.minElementGranularityNFrames = 1;
	i.maxNContiguousFrames = 1;

	// Don't allocate frames beyond what the DMA engine can address.
	i.addressableBits = __PADDR_NBITS__;
	i.beyondEndPfn = 0;
	i.beyondEndPfn = ~i.beyondEndPfn;
	tmpAttr = cons->getAttr(UDI_DMA_DATA_ADDRESSABLE_BITS);
	if (tmpAttr != NULL)
	{
		i.addressableBits = tmpAttr->attr_value;
		// Clamp addressableBits down to __PADDR_NBITS__.
		if (i.addressableBits > __PADDR_NBITS__) {
			i.addressableBits = __PADDR_NBITS__;
		};

		if (i.addressableBits >= PAGING_BASE_SHIFT)
		{
			i.beyondEndPfn = paddr_t(1)
				<< (i.addressableBits - PAGING_BASE_SHIFT);
		}
		else {
			/* If the engine can only address the first frame, then
			 * limit allocation to the first frame
			 */
			i.beyondEndPfn = 1;
		};
	};

	/* Skip stride is # to skip - 1, so that we can always do
	 * a += (1 + stride) in the loop bound increment step.
	 * Skip stride ensures we meet element alignment requirements.
	 */
	i.pfnSkipStride = 0;
	tmpAttr = cons->getAttr(UDI_DMA_ELEMENT_ALIGNMENT_BITS);
	if (tmpAttr != NULL && tmpAttr->attr_value > PAGING_BASE_SHIFT) {
		i.pfnSkipStride = (1 << (tmpAttr->attr_value - PAGING_BASE_SHIFT)) - 1;
	};

	// 0 means unlimited elements per segment.
	i.maxElementsPerSegment = 0;
	tmpAttr = cons->getAttr(UDI_DMA_ELEMENT_ALIGNMENT_BITS);
	if (tmpAttr != NULL) {
		i.maxElementsPerSegment = tmpAttr->attr_value;
	}

	i.startPfn = 0;
	tmpAttr = cons->getAttr(UDI_DMA_ADDR_FIXED_BITS);
	if (tmpAttr != NULL && tmpAttr->attr_value > 0)
	{
		paddr_t					beyondEndOfFixedPfn;
		const udi_dma_constraints_attr_spec_t	*tmpFixedType,
							*tmpFixedLo,*tmpFixedHi;

		/* We don't need to track this value since we can just constrain
		 * startPfn and then discard it, but track it anyway.
		 */
		i.fixedBits = tmpAttr->attr_value;

		tmpFixedType = cons->getAttr(UDI_DMA_ADDR_FIXED_TYPE);
		if (tmpFixedType != NULL && tmpFixedType->attr_value == UDI_DMA_FIXED_LIST)
		{
			printf(WARNING MEMBMP"constrainedGF: "
				"FIXED_LIST unsupported. Rejecting.\n");

			return ERROR_UNSUPPORTED;
		};

		/* We ignore UDI_DMA_FIXED_ELEMENT because it has no practical
		 * meaning, as far as I can tell.
		 */
		if (tmpFixedType != NULL && tmpFixedType->attr_value == UDI_DMA_FIXED_ELEMENT)
		{
			printf(WARNING MEMBMP"constrainedGF: FIXED_ELEMENT "
				"silently ignored.\n");
		};

		if (tmpFixedType != NULL && tmpFixedType->attr_value == UDI_DMA_FIXED_VALUE)
		{
			/**	EXPLANATION:
			 * Extract the fixed bits in a PAE-compatible way, and
			 * use them to determine what changes need to be made to the
			 * start and beyondEnd PFNs.
			 */
			tmpFixedLo = cons->getAttr(UDI_DMA_ADDR_FIXED_VALUE_LO);
			tmpFixedHi = cons->getAttr(UDI_DMA_ADDR_FIXED_VALUE_HI);

#ifndef CONFIG_ARCH_x86_32_PAE
			if (tmpFixedHi != NULL && tmpFixedHi->attr_value != 0) {
				printf(ERROR MEMBMP"constrainedGF: Fixed value "
					"hi constraint in a non-PAE build.\n\t"
					"Unsupported. Hi value is %x.\n",
					tmpFixedHi->attr_value);

				return ERROR_UNSUPPORTED;
			};
#endif

			i.fixedBitsValue = paddr_t(
#ifdef CONFIG_ARCH_x86_32_PAE
				((tmpFixedHi != NULL) ? tmpFixedHi->attr_value : 0),
#endif
				((tmpFixedLo != NULL) ? tmpFixedLo->attr_value : 0));

			i.startPfn = (i.fixedBitsValue << (i.addressableBits - i.fixedBits))
				>> PAGING_BASE_SHIFT;

			/* Because beyondEndOfFixedPfn is calculated relative to
			 * startPfn, we don't have to check later on to see if
			 * beyondEndOfFixedPfn is before startPfn.
			 */
			beyondEndOfFixedPfn = ((i.fixedBitsValue + 1)
				<< (i.addressableBits - i.fixedBits))
				>> PAGING_BASE_SHIFT;

			/* We also have to modify beyondEndPfn based on the
			 * fixed bits: the fixed bits' beyond-end PFN
			 * will be our new beyondEndPfn, as long as it's
			 * not outside the range of this BMP.
			 *
			 *	XXX:
			 * Don't need to check if beyondEndOfFixedPfn is before
			 * startPfn, because addressableBits was already checked.
			 */
			if (beyondEndOfFixedPfn > i.beyondEndPfn) {
				beyondEndOfFixedPfn = i.beyondEndPfn;
			}
			i.beyondEndPfn = beyondEndOfFixedPfn;
		};

	};

	/* There's nothing really to check for here. Perhaps we could check to
	 * ensure that the element granularity wouldn't cause the first attempt
	 * to cross beyond the end of the BMP? Other than that, no harm
	 * can come from values in this attribute.
	 */
	tmpAttr = cons->getAttr(UDI_DMA_ELEMENT_GRANULARITY_BITS);
	if (tmpAttr != NULL && tmpAttr->attr_value > PAGING_BASE_SHIFT)
	{
		i.minElementGranularityNFrames = 1
			<< (tmpAttr->attr_value - PAGING_BASE_SHIFT);

		/* Max N Contiguous frames should be updated to match the min
		 * granularity, and then it will be re-constrained later when we
		 * process UDI_DMA_ELEMENT_LENGTH_BITS, if one is supplied.
		 */
		i.maxNContiguousFrames = i.minElementGranularityNFrames;
	};

	tmpAttr = cons->getAttr(UDI_DMA_ELEMENT_LENGTH_BITS);
	if (tmpAttr != NULL) {
		ubit32		maxElemBits=tmpAttr->attr_value;

		if (maxElemBits < PAGING_BASE_SHIFT) {
			maxElemBits = PAGING_BASE_SHIFT;
		};
		maxElemBits -= PAGING_BASE_SHIFT;
		i.maxNContiguousFrames = 1 << maxElemBits;
	}

	// maxNContiguousFrames cannot be less than minElementGranularityNFrames.
	if (i.maxNContiguousFrames < i.minElementGranularityNFrames)
	{
		printf(ERROR MEMBMP"constrainedGF: Minimum granularity is %d "
			"frames, but max element size is %d frames.\n",
			i.minElementGranularityNFrames, i.maxNContiguousFrames);

		return ERROR_INVALID_ARG_VAL;
	};

	/* We don't yet support slop. Especially not SLOP-IN. */
	i.slopInBits = i.slopOutBits = i.slopOutExtra = 0;

	tmpAttr = cons->getAttr(UDI_DMA_SLOP_IN_BITS);
	if (tmpAttr != NULL) { i.slopInBits = tmpAttr->attr_value; };
	tmpAttr = cons->getAttr(UDI_DMA_SLOP_OUT_BITS);
	if (tmpAttr != NULL) { i.slopOutBits = tmpAttr->attr_value; };
	tmpAttr = cons->getAttr(UDI_DMA_SLOP_OUT_EXTRA);
	if (tmpAttr != NULL) { i.slopOutExtra = tmpAttr->attr_value; };

	if (i.slopInBits != 0 || i.slopOutBits != 0 || i.slopOutExtra != 0)
	{
		printf(ERROR MEMBMP"constrainedGF: SLOP is unsupported\n"
			"\tSLOP-IN %d, SLOP-OUT %d, SLOP-EXTRA %d.\n",
			i.slopInBits, i.slopOutBits, i.slopOutExtra);

		return ERROR_UNSUPPORTED;
	};

	tmpAttr = cons->getAttr(UDI_DMA_NO_PARTIAL);
	if (tmpAttr != NULL && tmpAttr->attr_value != 0) {
		i.partialAllocationsDisallowed = 1;
	};

	tmpAttr = cons->getAttr(UDI_DMA_SEQUENTIAL);
	if (tmpAttr != NULL && tmpAttr->attr_value != 0) {
		i.sequentialAccessHint = 1;
	};

	/* SCGTH_FORMAT *must* be provided. */
	i.callerSupportedFormats = 0;
	tmpAttr = cons->getAttr(UDI_DMA_SCGTH_FORMAT);
	if (tmpAttr == NULL)
	{
		printf(ERROR"Constraints compiler: No SCGTH_FORMAT provided.\n");
		return ERROR_NON_CONFORMANT;
	}
	else
	{
		if ((tmpAttr->attr_value & (UDI_SCGTH_32 | UDI_SCGTH_64)) == 0)
		{
			printf(ERROR"Constraints compiler: SCGTH_FORMAT "
				"doesn't specify any list word-sizes.\n");
			return ERROR_NON_CONFORMANT;
		};

		/* Store the caller's indicated supported formats.
		 * The final choice on which list element format is actually
		 * returned by the kernel to the user, is made at point of list
		 * copy-out in getElements().
		 *
		 * In particular, notice how `callerSupportedFormats` is
		 * actually never used by the kernel itself in making that
		 * decision.
		 *
		 * `callerSupportedFormats` is for userspace's benefit. It
		 * allows the libzudi code in userspace to see what formats were
		 * indicated by the uncompiled constraints that were originally
		 * bound to the sglist object (i.e, those constraints which we
		 * are presumably compiling right now in this function
		 * invocation).
		 */
		i.callerSupportedFormats = tmpAttr->attr_value;
	};

	return ERROR_SUCCESS;
}
