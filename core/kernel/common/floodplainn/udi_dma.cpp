
#include <arch/walkerPageRanger.h>
#include <kernel/common/memoryTrib/vaddrSpaceStream.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/floodplainn/zudi.h>


utf8Char *fplainn::Zudi::dma::DmaConstraints::attrTypeNames[32] =
{
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

utf8Char *fplainn::Zudi::dma::DmaConstraints::getAttrTypeName(
	udi_dma_constraints_attr_t a
	)
{
	if (a == 0 || a > UDI_DMA_SLOP_BARRIER_BITS) { return unknownString; };

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

	return unknownString;
}

void fplainn::Zudi::dma::DmaConstraints::dump(void)
{
	printf(NOTICE"DMA Constraints obj @0x%p, %d attrs: dumping.\n",
		this, attrs.getNIndexes());

	for (AttrArray::Iterator it=attrs.begin(); it != attrs.end(); ++it)
	{
		udi_dma_constraints_attr_spec_t		*tmp=&*it;

		printf(CC"\tAttr %s,\t\tValue 0x%x.\n",
			getAttrTypeName(tmp->attr_type),
			tmp->attr_value);
	};
}

error_t fplainn::Zudi::dma::DmaConstraints::addOrModifyAttrs(
	udi_dma_constraints_attr_spec_t *_attrs, uarch_t nAttrs
	)
{
	error_t			ret;
	uarch_t			nNewAttrs=0;

	if (_attrs == NULL) { return ERROR_INVALID_ARG; };

	// How many of the attrs are new ones?
	for (uarch_t i=0; i<nAttrs; i++) {
		if (!attrAlreadySet(_attrs[i].attr_type)) { nNewAttrs++; };
	};

	// Make room for all the new attrs.
	if (nNewAttrs > 0)
	{
		uarch_t			prevNIndexes = attrs.getNIndexes();

		ret = attrs.resizeToHoldIndex(
			prevNIndexes + nNewAttrs - 1);

		if (ret != ERROR_SUCCESS) { return ret; };

		// Set all the new attrs' attr_types to 0 to distinguish them.
		for (uarch_t i=0; i<nNewAttrs; i++) {
			attrs[prevNIndexes + i].attr_type = 0;
		};
	};

	for (AttrArray::Iterator it=attrs.begin(); it != attrs.end(); ++it)
	{
		udi_dma_constraints_attr_spec_t		*spec=&*it;

		/* If it's a newly allocated attr (attr_type=0), search the
		 * passed attrs for one that's new, and put it in this slot.
		 **/
		if (spec->attr_type == 0)
		{
			for (uarch_t i=0; i<nAttrs; i++)
			{
				if (attrAlreadySet(_attrs[i].attr_type))
					{ continue; };

				*spec = _attrs[i];
			};
		}
		/* Else search the passed attrs to see if the caller wanted this
		 * attr modified.
		 **/
		else
		{
			for (uarch_t i=0; i<nAttrs; i++)
			{
				if (spec->attr_type != _attrs[i].attr_type)
					{ continue; };

				spec->attr_value = _attrs[i].attr_value;
			};
		};
	};

	return ERROR_SUCCESS;
}

error_t fplainn::Zudi::dma::ScatterGatherList::map(
	MappedScatterGatherList *retobj
	)
{
	status_t		ret;
	uarch_t			nFrames=0;
	void			*vmem=NULL;
	uintptr_t		currVaddr=0;
	VaddrSpaceStream	*__kvasStream;

	/**	EXPLANATION:
	 * First count the number of pages we have to allocate in order to
	 * fully map the entire SGList. Then allocate the vmem to map it,
	 * and well, map it.
	 **/
	__kvasStream = processTrib.__kgetStream()->getVaddrSpaceStream();

	for (ubit8 pass=1; pass <= 2; pass++)
	{
		if (addressSize == ADDR_SIZE_32)
		{
			for (
				SGList32::Iterator it=elements32.begin();
				it != elements32.end();
				++it)
			{
				udi_scgth_element_32_t		*tmp = &*it;
				paddr_t				p, p2;
				uarch_t				currNFrames;

				p = tmp->block_busaddr;
				p2 = p + tmp->block_length;

				currNFrames = PAGING_BYTES_TO_PAGES(p2 - p)
					.getLow();

				if (pass == 1) {
					nFrames += currNFrames;
				};

				if (pass == 2 && vmem == NULL)
				{
					// Alloc the vmem.
					vmem = __kvasStream->getPages(nFrames);
					if (vmem == NULL)
					{
						printf(NOTICE"SGList::map: "
							"Failed to alloc vmem "
							"to map the %d "
							"frames in the list.\n",
							nFrames);

						return ERROR_MEMORY_NOMEM_VIRTUAL;
					};

					currVaddr = (uintptr_t)vmem;
				};

				if (pass == 2)
				{
					ret = walkerPageRanger::mapInc(
						&__kvasStream->vaddrSpace,
						(void *)currVaddr, p,
						currNFrames,
						PAGEATTRIB_PRESENT
						| PAGEATTRIB_WRITE
						| PAGEATTRIB_SUPERVISOR);

					if (ret < (signed)currNFrames)
					{
						printf(NOTICE"SGList::map: "
							"Failed to map all %d "
							"pages for SGList "
							"element P0x%P, %d "
							"frames.\n",
							&p, nFrames);

						goto releaseVmem;
					};

					currVaddr += currNFrames
						* PAGING_BASE_SIZE;
				};
			};
		}
		else
		{
			for (
				SGList64::Iterator it=elements64.begin();
				it != elements64.end();
				++it)
			{
				udi_scgth_element_64_t		*tmp = &*it;
				paddr_t				p, p2;
				uarch_t				currNFrames;

				assign_busaddr64_to_paddr(
					p, tmp->block_busaddr);

				p2 = p + tmp->block_length;

				currNFrames = PAGING_BYTES_TO_PAGES(p2 - p)
					.getLow();

				nFrames += currNFrames;

				if (pass == 1) {
					nFrames += currNFrames;
				};

				if (pass == 2 && vmem == NULL)
				{
					// Alloc the vmem.
					vmem = __kvasStream->getPages(nFrames);
					if (vmem == NULL)
					{
						printf(NOTICE"SGList::map: "
							"Failed to alloc vmem "
							"to map the %d "
							"frames in the list.\n",
							nFrames);

						return ERROR_MEMORY_NOMEM_VIRTUAL;
					};

					currVaddr = (uintptr_t)vmem;
				};

				if (pass == 2)
				{
					ret = walkerPageRanger::mapInc(
						&__kvasStream->vaddrSpace,
						(void *)currVaddr, p,
						currNFrames,
						PAGEATTRIB_PRESENT
						| PAGEATTRIB_WRITE
						| PAGEATTRIB_SUPERVISOR);

					if (ret < (signed)currNFrames)
					{
						printf(NOTICE"SGList::map: "
							"Failed to map all %d "
							"pages for SGList "
							"element P0x%P, %d "
							"frames.\n",
							&p, nFrames);

						goto releaseVmem;
					};

					currVaddr += currNFrames
						* PAGING_BASE_SIZE;
				};
			};
		};
	};

	/** FIXME: Build the page list here.
	 **/
	return ERROR_SUCCESS;

releaseVmem:
	__kvasStream->releasePages(vmem, nFrames);
	return ERROR_MEMORY_VIRTUAL_PAGEMAP;
}
