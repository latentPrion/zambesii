
#include <commonlibs/libacpi/madt.h>


acpi_rMadtCpuS *acpiRMadt::getNextCpuEntry(
	acpi_rMadtS *madt, void **const handle
	)
{
	acpi_rMadtCpuS		*ret=NULL;

	if (*handle == NULL) {
		*handle = ACPI_MADT_GET_FIRST_ENTRY(madt);
	};

	while (*handle < ACPI_TABLE_GET_ENDADDR(madt))
	{
		switch (ACPI_MADT_GET_TYPE(*handle))
		{
		case ACPI_MADT_TYPE_LAPIC:
			ret = static_cast<acpi_rMadtCpuS *>( *handle );
			*handle = ACPI_PTR_INC_BY(
				*handle, ((acpi_rMadtCpuS *)*handle)->length);

			return ret;


		default:
			// Use the 'length' member to skip over all others.
			*handle = ACPI_PTR_INC_BY(
				*handle, ((acpi_rMadtCpuS *)*handle)->length);

			break;
		};
	};

	return NULL;
}

acpi_rMadtIoApicS *acpiRMadt::getNextIoApicEntry(
	acpi_rMadtS *madt, void **const handle
	)
{
	acpi_rMadtIoApicS	*ret=NULL;

	if (*handle == NULL) {
		*handle = ACPI_MADT_GET_FIRST_ENTRY(madt);
	};

	while (*handle < ACPI_TABLE_GET_ENDADDR(madt))
	{
		switch (ACPI_MADT_GET_TYPE(*handle))
		{
		case ACPI_MADT_TYPE_IOAPIC:
			ret = static_cast<acpi_rMadtIoApicS *>( *handle );
			*handle = ACPI_PTR_INC_BY(
				*handle, ((acpi_rMadtCpuS *)*handle)->length);

			return ret;

		default:
			// Use the 'length' member to skip over all others.
			*handle = ACPI_PTR_INC_BY(
				*handle, ((acpi_rMadtCpuS *)*handle)->length);

			break;
		};
	};

	return NULL;
}

acpi_rMadtLapicNmiS *acpiRMadt::getNextLapicNmiEntry(
	acpi_rMadtS *madt, void **const handle
	)
{
	acpi_rMadtLapicNmiS	*ret=NULL;

	if (*handle == NULL) {
		*handle = ACPI_MADT_GET_FIRST_ENTRY(madt);
	};

	while (*handle < ACPI_TABLE_GET_ENDADDR(madt))
	{
		switch (ACPI_MADT_GET_TYPE(*handle))
		{
		case ACPI_MADT_TYPE_LAPIC_NMI:
			ret = static_cast<acpi_rMadtLapicNmiS *>( *handle );
			*handle = ACPI_PTR_INC_BY(
				*handle, ((acpi_rMadtCpuS *)*handle)->length);

			return ret;

		default:
			// Use the 'length' member to skip over all others.
			*handle = ACPI_PTR_INC_BY(
				*handle, ((acpi_rMadtCpuS *)*handle)->length);

			break;
		};
	};

	return NULL;
}

acpi_rMadtIrqSourceOverS *acpiRMadt::getNextIrqSourceOverrideEntry(
	acpi_rMadtS *madt, void **const handle
	)
{
	acpi_rMadtIrqSourceOverS	*ret=NULL;

	if (*handle == NULL) {
		*handle = ACPI_MADT_GET_FIRST_ENTRY(madt);
	};

	while (*handle < ACPI_TABLE_GET_ENDADDR(madt))
	{
		switch (ACPI_MADT_GET_TYPE(*handle))
		{
		case ACPI_MADT_TYPE_IRQSOURCE_OVERRIDE:
			ret = static_cast<acpi_rMadtIrqSourceOverS*>( *handle );
			*handle = ACPI_PTR_INC_BY(
				*handle, ((acpi_rMadtCpuS *)*handle)->length);

			return ret;

		default:
			// Use the 'length' member to skip over all others.
			*handle = ACPI_PTR_INC_BY(
				*handle, ((acpi_rMadtCpuS *)*handle)->length);

			break;
		};
	};

	return NULL;
}

