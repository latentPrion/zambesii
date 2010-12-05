
#include <commonlibs/libacpi/srat.h>


acpi_rSratCpuS *acpiRSrat::getNextCpuEntry(
	acpi_rSratS *srat, void **const handle
	)
{
	acpi_rSratCpuS		*ret=__KNULL;

	if (*handle == __KNULL) {
		*handle = ACPI_SRAT_GET_FIRST_ENTRY(srat);
	};

	while (*handle < ACPI_SRAT_GET_ENDADDR(srat))
	{
		switch (ACPI_SRAT_GET_TYPE(*handle))
		{
		case ACPI_SRAT_TYPE_CPU:
			ret = static_cast<acpi_rSratCpuS *>( *handle );
			*handle = ACPI_PTR_INC_BY(*handle, acpi_rSratCpuS);
			return ret;

		default:
			// Use the 'length' member to skip over all others.
			*handle = reinterpret_cast<void *>(
				(uarch_t)*handle
					+ (((acpi_rSratCpuS *)*handle)
					->length));

			break;
		};
	};

	return __KNULL;
}

acpi_rSratMemS *acpiRSrat::getNextMemEntry(
	acpi_rSratS *srat, void **const handle
	)
{
	acpi_rSratMemS		*ret=__KNULL;

	if (*handle == __KNULL) {
		*handle = ACPI_SRAT_GET_FIRST_ENTRY(srat);
	};

	while (*handle < ACPI_SRAT_GET_ENDADDR(srat))
	{
		switch (ACPI_SRAT_GET_TYPE(*handle))
		{
		case ACPI_SRAT_TYPE_MEM:
			ret = static_cast<acpi_rSratMemS *>( *handle );
			*handle = ACPI_PTR_INC_BY(*handle, acpi_rSratMemS);
			return ret;

		default:
			// Use the 'length' member to skip over all others.
			*handle = reinterpret_cast<void *>(
				(uarch_t)*handle
					+ (((acpi_rSratMemS *)*handle)
					->length));

			break;
		};
	};

	return __KNULL;
}

