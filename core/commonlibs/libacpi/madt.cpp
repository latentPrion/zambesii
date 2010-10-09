
#include <commonlibs/libacpi/madt.h>


acpi_rMadtCpuS *acpiRMadt::getNextCpuEntry(
	acpi_rMadtS *madt, void **const handle
	)
{
	acpi_rMadtCpuS		*ret=__KNULL;

	if (*handle == __KNULL) {
		*handle = ACPI_MADT_GET_FIRST_ENTRY(madt);
	};

	while (*handle < ACPI_MADT_GET_ENDADDR(madt))
	{
		switch (ACPI_MADT_GET_TYPE(*handle))
		{
		case ACPI_MADT_TYPE_LAPIC:
			ret = static_cast<acpi_rMadtCpuS *>( *handle );
			*handle = ACPI_PTR_INC_BY(*handle, acpi_rMadtCpuS);
			return ret;

		default:
			// Use the 'length' member to skip over all others.
			*handle = reinterpret_cast<void *>(
				(uarch_t)*handle
					+ (((acpi_rMadtCpuS *)*handle)
					->length));

			break;
		};
	};

	return __KNULL;
}

acpi_rMadtIoApicS *acpiRMadt::getNextIoApicEntry(
	acpi_rMadtS *madt, void **const handle
	)
{
	acpi_rMadtIoApicS	*ret=__KNULL;

	if (*handle == __KNULL) {
		*handle = ACPI_MADT_GET_FIRST_ENTRY(madt);
	};

	while (*handle < ACPI_MADT_GET_ENDADDR(madt))
	{
		switch (ACPI_MADT_GET_TYPE(*handle))
		{
		case ACPI_MADT_TYPE_IOAPIC:
			ret = static_cast<acpi_rMadtIoApicS *>( *handle );
			*handle = ACPI_PTR_INC_BY(*handle, acpi_rMadtIoApicS);
			return ret;

		default:
			// Use the 'length' member to skip over all others.
			*handle = reinterpret_cast<void *>(
				(uarch_t)*handle
					+ (((acpi_rMadtCpuS *)*handle)
					->length));

			break;
		};
	};

	return __KNULL;
}

