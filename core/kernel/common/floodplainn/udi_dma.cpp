
#include <arch/walkerPageRanger.h>
#include <__kstdlib/__kclib/assert.h>
#include <kernel/common/panic.h>
#include <kernel/common/memoryTrib/vaddrSpaceStream.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/processTrib/processTrib.h>
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
	error_t			ret;
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

error_t fplainn::dma::MappedScatterGatherList::addPages(
	void *vaddr, uarch_t nBytes
	)
{
	uarch_t nPages;
	error_t ret;

	assert_warn(nBytes % PAGING_BASE_SIZE == 0);
	nPages = __KMATH_NELEMENTS(nBytes, PAGING_BASE_SIZE);

	ret = pageArray.resizeToHoldIndex(pageArray.getNIndexes() + nPages - 1);
	if (ret != ERROR_SUCCESS) {
		return ret;
	}

	for (uarch_t i=0; i<nPages; i++)
	{
		pageArray[i] = (void *)((uintptr_t)vaddr
			+ (i * PAGING_BASE_SIZE));
	};

	return ERROR_SUCCESS;
}

template <class scgth_elements_type>
error_t fplainn::dma::ScatterGatherList::map(
	ResizeableArray<scgth_elements_type> *list,
	MappedScatterGatherList *retobj
	)
{
	status_t		ret;
	uarch_t			nFrames=0;
	void			*vmem=NULL;
	uintptr_t		currVaddr=0;
	VaddrSpaceStream	*currVasStream;

	/**	EXPLANATION:
	 * First count the number of pages we have to allocate in order to
	 * fully map the entire SGList. Then allocate the vmem to map it,
	 * and well, map it.
	 **/
	if (retobj == NULL) { return ERROR_INVALID_ARG_VAL; };

	currVasStream = cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentThread()->parent->getVaddrSpaceStream();

	for (ubit8 pass=1; pass <= 2; pass++)
	{
		for (
			typename ResizeableArray<scgth_elements_type>::Iterator it=list->begin();
			it != list->end();
			++it)
		{
			scgth_elements_type		*tmp = &*it;
			paddr_t				p, p2;
			uarch_t				currNFrames;

			assign_scgth_block_busaddr_to_paddr(
				p, tmp->block_busaddr);

			p2 = p + tmp->block_length;

			currNFrames = PAGING_BYTES_TO_PAGES(p2 - p)
				.getLow();

			if (pass == 1) {
				nFrames += currNFrames;
			};

			if (pass == 2)
			{
				if (vmem == NULL)
				{
					// Alloc the vmem.
					vmem = currVasStream->getPages(nFrames);
					if (vmem == NULL)
					{
						printf(ERROR"SGList::map: "
							"Failed to alloc vmem "
							"to map the %d "
							"frames in the list.\n",
							nFrames);

						return ERROR_MEMORY_NOMEM_VIRTUAL;
					};

					currVaddr = (uintptr_t)vmem;
				}

				ret = walkerPageRanger::mapInc(
					&currVasStream->vaddrSpace,
					(void *)currVaddr, p,
					currNFrames,
					PAGEATTRIB_PRESENT
					| PAGEATTRIB_WRITE
					| PAGEATTRIB_SUPERVISOR);

				if (ret < (signed)currNFrames)
				{
					printf(ERROR"SGList::map: "
						"Failed to map all %d "
						"pages for SGList "
						"element P%P, %d "
						"frames.\n",
						&p, nFrames);

					goto releaseVmem;
				};

				ret = retobj->addPages(
					(void *)currVaddr,
					currNFrames * PAGING_BASE_SIZE);

				if (ret != ERROR_SUCCESS)
				{
					printf(ERROR"SGList::map: "
						"Failed to add "
						"page %p in DMA "
						"sglist mapping to "
						"internal metadata "
						"list.\n",
					(void*)currVaddr);
				}

				currVaddr += currNFrames
					* PAGING_BASE_SIZE;
			};
		};
	};

	/**	FIXME: Build the page list here.
	 **/

	return ERROR_SUCCESS;

releaseVmem:
	currVasStream->releasePages(vmem, nFrames);
	return ERROR_MEMORY_VIRTUAL_PAGEMAP;
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
	printf(NOTICE"DMACon Compiler @%p: Parent @%p. Dumping.\n"
		"\tCan address %u bits, will be presented with %s-bit descriptors.\n"
		"\tstarting at PFN %x upto PFN %x.\n"
		"\tSkip stride of %d frames.\n"
		"\tAllocates in blocks of at least %d frames, up to a max of %u frames per elem.\n"
		"\tSlop in is %d bits, out is %d bits.\n"
		"\tPartial alloc? %s. Sequentially accessed? %s.\n",
		this, parent,
		i.addressableBits,
		((i.addressSize == scatterGatherLists::ADDR_SIZE_64) ? "64" : "32"),
		i.startPfn, i.beyondEndPfn - 1,
		i.pfnSkipStride,
		i.minElementGranularityNFrames, i.maxNContiguousFrames,
		i.slopInBits, i.slopOutBits,
		((i.partialAllocationsDisallowed) ? "no" : "yes"),
		((i.sequentialAccessHint) ? "yes" : "no"));
}

error_t fplainn::dma::constraints::Compiler::compile(void)
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
	tmpAttr = parent->getAttr(UDI_DMA_DATA_ADDRESSABLE_BITS);
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
	tmpAttr = parent->getAttr(UDI_DMA_ELEMENT_ALIGNMENT_BITS);
	if (tmpAttr != NULL && tmpAttr->attr_value > PAGING_BASE_SHIFT) {
		i.pfnSkipStride = (1 << (tmpAttr->attr_value - PAGING_BASE_SHIFT)) - 1;
	};

	i.startPfn = 0;
	tmpAttr = parent->getAttr(UDI_DMA_ADDR_FIXED_BITS);
	if (tmpAttr != NULL && tmpAttr->attr_value > 0)
	{
		paddr_t					beyondEndOfFixedPfn;
		const udi_dma_constraints_attr_spec_t	*tmpFixedType,
							*tmpFixedLo,*tmpFixedHi;

		/* We don't need to track this value since we can just constrain
		 * startPfn and then discard it, but track it anyway.
		 */
		i.fixedBits = tmpAttr->attr_value;

		tmpFixedType = parent->getAttr(UDI_DMA_ADDR_FIXED_TYPE);
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
			tmpFixedLo = parent->getAttr(UDI_DMA_ADDR_FIXED_VALUE_LO);
			tmpFixedHi = parent->getAttr(UDI_DMA_ADDR_FIXED_VALUE_HI);

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
	tmpAttr = parent->getAttr(UDI_DMA_ELEMENT_GRANULARITY_BITS);
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

	tmpAttr = parent->getAttr(UDI_DMA_ELEMENT_LENGTH_BITS);
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

	tmpAttr = parent->getAttr(UDI_DMA_SLOP_IN_BITS);
	if (tmpAttr != NULL) { i.slopInBits = tmpAttr->attr_value; };
	tmpAttr = parent->getAttr(UDI_DMA_SLOP_OUT_BITS);
	if (tmpAttr != NULL) { i.slopOutBits = tmpAttr->attr_value; };
	tmpAttr = parent->getAttr(UDI_DMA_SLOP_OUT_EXTRA);
	if (tmpAttr != NULL) { i.slopOutExtra = tmpAttr->attr_value; };

	if (i.slopInBits != 0 || i.slopOutBits != 0 || i.slopOutExtra != 0)
	{
		printf(ERROR MEMBMP"constrainedGF: SLOP is unsupported\n"
			"\tSLOP-IN %d, SLOP-OUT %d, SLOP-EXTRA %d.\n",
			i.slopInBits, i.slopOutBits, i.slopOutExtra);

		return ERROR_UNSUPPORTED;
	};

	tmpAttr = parent->getAttr(UDI_DMA_NO_PARTIAL);
	if (tmpAttr != NULL && tmpAttr->attr_value != 0) {
		i.partialAllocationsDisallowed = 1;
	};

	tmpAttr = parent->getAttr(UDI_DMA_SEQUENTIAL);
	if (tmpAttr != NULL && tmpAttr->attr_value != 0) {
		i.sequentialAccessHint = 1;
	};

	/* SCGTH_FORMAT *must* be provided. */
	i.addressSize = scatterGatherLists::ADDR_SIZE_UNKNOWN;
	tmpAttr = parent->getAttr(UDI_DMA_SCGTH_FORMAT);
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

		/*	EXPLANATION:
		 * On a 32-bit build, prefer to use 32-bit SCGTH, unless
		 * it's a PAE build.
		 *
		 * The reason we prefer to do 64-bit scgth lists on a
		 * PAE build is because it makes it more likely that
		 * device DMA memory will be allocated from the higher
		 * regions in RAM, away from where most application
		 * footprint is.
		 *
		 * On a 64-bit build we also prefer to use 64-bit DMA so we have
		 * a higher chance of reserving the low 32-bits for devices that
		 * truly need 32-bit DMA.
		 */

		/*	NOTES:
		 * We allow 64-bit formatted scgth lists on 32 bit builds
		 * because format of the list is not the same as the range of
		 * pmem from which the memory was allocated.
		 *
		 * So we can allocate memory in the first 4GiB to a caller that
		 * requests a 64-bit formatted list. This just means we set the
		 * top 32-bit of the addresses to 0.
		 **/
#if (__VADDR_NBITS__ == 64) || ((__VADDR_NBITS__ == 32) && defined(ARCH_x86_32_PAE))
		if (tmpAttr->attr_value & UDI_SCGTH_64) {
			i.addressSize = scatterGatherLists::ADDR_SIZE_64;
		} else {
			i.addressSize = scatterGatherLists::ADDR_SIZE_32;
		};
#elif (__VADDR_NBITS__ == 32)
		if (tmpAttr->attr_value & UDI_SCGTH_32) {
			i.addressSize = scatterGatherLists::ADDR_SIZE_32;
		} else {
			i.addressSize = scatterGatherLists::ADDR_SIZE_64;
		};
#else
	#error "Unhandled build configuration for UDI DMA SCGTH_FORMAT."
#endif
	};

	return ERROR_SUCCESS;
}
