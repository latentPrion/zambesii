
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
	assert_fatal(addressSize != scatterGatherLists::ADDR_SIZE_UNKNOWN);

	unmap();

	if (!justTransfer) {
		freeSGListElements();
	}

	compiledConstraints.setInvalid();
	addressSize = scatterGatherLists::ADDR_SIZE_UNKNOWN;
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
