
#include <commonlibs/libacpi/madt.h>


acpiR::madt::sCpu *acpiR::madt::getNextCpuEntry(
	acpiR::sMadt *madt, void **const handle
	)
{
	acpiR::madt::sCpu		*ret=NULL;

	if (*handle == NULL) {
		*handle = ACPI_MADT_GET_FIRST_ENTRY(madt);
	};

	while (*handle < ACPI_TABLE_GET_ENDADDR(madt))
	{
		switch (ACPI_MADT_GET_TYPE(*handle))
		{
		case ACPI_MADT_TYPE_LAPIC:
			ret = static_cast<acpiR::madt::sCpu *>( *handle );
			*handle = ACPI_PTR_INC_BY(
				*handle, ((acpiR::madt::sCpu *)*handle)->length);

			return ret;


		default:
			// Use the 'length' member to skip over all others.
			*handle = ACPI_PTR_INC_BY(
				*handle, ((acpiR::madt::sCpu *)*handle)->length);

			break;
		};
	};

	return NULL;
}

acpiR::madt::sIoApic *acpiR::madt::getNextIoApicEntry(
	acpiR::sMadt *madt, void **const handle
	)
{
	acpiR::madt::sIoApic	*ret=NULL;

	if (*handle == NULL) {
		*handle = ACPI_MADT_GET_FIRST_ENTRY(madt);
	};

	while (*handle < ACPI_TABLE_GET_ENDADDR(madt))
	{
		switch (ACPI_MADT_GET_TYPE(*handle))
		{
		case ACPI_MADT_TYPE_IOAPIC:
			ret = static_cast<acpiR::madt::sIoApic *>( *handle );
			*handle = ACPI_PTR_INC_BY(
				*handle, ((acpiR::madt::sCpu *)*handle)->length);

			return ret;

		default:
			// Use the 'length' member to skip over all others.
			*handle = ACPI_PTR_INC_BY(
				*handle, ((acpiR::madt::sCpu *)*handle)->length);

			break;
		};
	};

	return NULL;
}

acpiR::madt::sLapicNmi *acpiR::madt::getNextLapicNmiEntry(
	acpiR::sMadt *madt, void **const handle
	)
{
	acpiR::madt::sLapicNmi	*ret=NULL;

	if (*handle == NULL) {
		*handle = ACPI_MADT_GET_FIRST_ENTRY(madt);
	};

	while (*handle < ACPI_TABLE_GET_ENDADDR(madt))
	{
		switch (ACPI_MADT_GET_TYPE(*handle))
		{
		case ACPI_MADT_TYPE_LAPIC_NMI:
			ret = static_cast<acpiR::madt::sLapicNmi *>( *handle );
			*handle = ACPI_PTR_INC_BY(
				*handle, ((acpiR::madt::sCpu *)*handle)->length);

			return ret;

		default:
			// Use the 'length' member to skip over all others.
			*handle = ACPI_PTR_INC_BY(
				*handle, ((acpiR::madt::sCpu *)*handle)->length);

			break;
		};
	};

	return NULL;
}

acpiR::madt::sIrqSourceOver *acpiR::madt::getNextIrqSourceOverrideEntry(
	acpiR::sMadt *madt, void **const handle
	)
{
	acpiR::madt::sIrqSourceOver	*ret=NULL;

	if (*handle == NULL) {
		*handle = ACPI_MADT_GET_FIRST_ENTRY(madt);
	};

	while (*handle < ACPI_TABLE_GET_ENDADDR(madt))
	{
		switch (ACPI_MADT_GET_TYPE(*handle))
		{
		case ACPI_MADT_TYPE_IRQSOURCE_OVERRIDE:
			ret = static_cast<acpiR::madt::sIrqSourceOver*>( *handle );
			*handle = ACPI_PTR_INC_BY(
				*handle, ((acpiR::madt::sCpu *)*handle)->length);

			return ret;

		default:
			// Use the 'length' member to skip over all others.
			*handle = ACPI_PTR_INC_BY(
				*handle, ((acpiR::madt::sCpu *)*handle)->length);

			break;
		};
	};

	return NULL;
}

