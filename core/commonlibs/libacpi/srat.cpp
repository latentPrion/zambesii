
#include <__kclasses/debugPipe.h>
#include <commonlibs/libacpi/srat.h>


acpi_rSratCpuS *acpiRSrat::getNextCpuEntry(
	acpi_rSratS *srat, void **const handle
	)
{
	acpi_rSratCpuS		*ret=NULL;

	if (*handle == NULL) {
		*handle = ACPI_SRAT_GET_FIRST_ENTRY(srat);
	};

	while (*handle < ACPI_TABLE_GET_ENDADDR(srat))
	{
		switch (ACPI_SRAT_GET_TYPE(*handle))
		{
		case ACPI_SRAT_TYPE_CPU:
			ret = static_cast<acpi_rSratCpuS *>( *handle );
			*handle = ACPI_PTR_INC_BY(
				*handle, ((acpi_rSratMemS *)*handle)->length);

			return ret;

		default:
			// Use the 'length' member to skip over all others.
			*handle = ACPI_PTR_INC_BY(
				*handle, ((acpi_rSratMemS *)*handle)->length);

			break;
		};
	};

	return NULL;
}

acpi_rSratMemS *acpiRSrat::getNextMemEntry(
	acpi_rSratS *srat, void **const handle
	)
{
	acpi_rSratMemS		*ret=NULL;

	if (*handle == NULL) {
		*handle = ACPI_SRAT_GET_FIRST_ENTRY(srat);
	};

	while (*handle < ACPI_TABLE_GET_ENDADDR(srat))
	{
		switch (ACPI_SRAT_GET_TYPE(*handle))
		{
		case ACPI_SRAT_TYPE_MEM:
			ret = static_cast<acpi_rSratMemS *>( *handle );
			*handle = ACPI_PTR_INC_BY(
				*handle, ((acpi_rSratMemS *)*handle)->length);

			return ret;

		default:
			// Use the 'length' member to skip over all others.
			*handle = ACPI_PTR_INC_BY(
				*handle, ((acpi_rSratMemS *)*handle)->length);

			break;
		};
	};

	return NULL;
}

