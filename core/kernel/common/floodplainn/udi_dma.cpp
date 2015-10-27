
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

	if (a >= UDI_DMA_ELEMENT_ALIGNMENT_BITS)
		{ return attrTypeNames[a - UDI_DMA_ELEMENT_ALIGNMENT_BITS + 12]; };

	if (a >= UDI_DMA_SCGTH_ALIGNMENT_BITS)
		{ return attrTypeNames[a - UDI_DMA_SCGTH_ALIGNMENT_BITS + 9]; };

	if (a >= UDI_DMA_SCGTH_MAX_ELEMENTS)
		{ return attrTypeNames[a - UDI_DMA_SCGTH_MAX_ELEMENTS + 4]; };

	if (a >= UDI_DMA_DATA_ADDRESSABLE_BITS)
		{ return attrTypeNames[a - UDI_DMA_DATA_ADDRESSABLE_BITS + 2]; };

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
