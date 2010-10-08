
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

		case ACPI_MADT_TYPE_IOAPIC:
			*handle = ACPI_PTR_INC_BY(*handle, acpi_rMadtIoApicS);
			break;

		case ACPI_MADT_TYPE_IRQSOURCE_OVERRIDE:
			*handle = ACPI_PTR_INC_BY(
				*handle, acpi_rMadtIrqSourceOverS);

			break;

		case ACPI_MADT_TYPE_NMI:
			*handle = ACPI_PTR_INC_BY(*handle, acpi_rMadtNmiS);
			break;

		case ACPI_MADT_TYPE_LAPIC_NMI:
			*handle = ACPI_PTR_INC_BY(*handle, acpi_rMadtLapicNmiS);
			break;

		case ACPI_MADT_TYPE_LAPICADDR_OVERRIDE:
			*handle = ACPI_PTR_INC_BY(
				*handle, acpi_rMadtLapicPaddrOverS);

			break;

		case ACPI_MADT_TYPE_IOSAPIC:
			*handle = ACPI_PTR_INC_BY(*handle, acpi_rMadtIoSapicS);
			break;

		case ACPI_MADT_TYPE_LSAPIC:
			*handle = ACPI_PTR_INC_BY(*handle, acpi_rMadtLSapicS);
			break;

		case ACPI_MADT_TYPE_IRQSOURCE:
			*handle = ACPI_PTR_INC_BY(
				*handle, acpi_rMadtIrqSourceS);

			break;
		};
	};

	return __KNULL;
}

