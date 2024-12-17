
#include <__kclasses/debugPipe.h>
#include <commonlibs/libacpi/srat.h>


acpiR::srat::sCpu *acpiR::srat::getNextCpuEntry(
	acpiR::sSrat *srat, void **const handle
	)
{
	acpiR::srat::sCpu		*ret=NULL;

	if (*handle == NULL) {
		*handle = ACPI_SRAT_GET_FIRST_ENTRY(srat);
	};

	while (*handle < ACPI_TABLE_GET_ENDADDR(srat))
	{
		switch (ACPI_SRAT_GET_TYPE(*handle))
		{
		case ACPI_SRAT_TYPE_CPU:
			ret = static_cast<acpiR::srat::sCpu *>( *handle );
			*handle = ACPI_PTR_INC_BY(
				*handle, ((acpiR::srat::sMem *)*handle)->length);

			return ret;

		default:
			// Use the 'length' member to skip over all others.
			*handle = ACPI_PTR_INC_BY(
				*handle, ((acpiR::srat::sMem *)*handle)->length);

			break;
		};
	};

	return NULL;
}

acpiR::srat::sMem *acpiR::srat::getNextMemEntry(
	acpiR::sSrat *srat, void **const handle
	)
{
	acpiR::srat::sMem		*ret=NULL;

	if (*handle == NULL) {
		*handle = ACPI_SRAT_GET_FIRST_ENTRY(srat);
	};

	while (*handle < ACPI_TABLE_GET_ENDADDR(srat))
	{
		switch (ACPI_SRAT_GET_TYPE(*handle))
		{
		case ACPI_SRAT_TYPE_MEM:
			ret = static_cast<acpiR::srat::sMem *>( *handle );
			*handle = ACPI_PTR_INC_BY(
				*handle, ((acpiR::srat::sMem *)*handle)->length);

			return ret;

		default:
			// Use the 'length' member to skip over all others.
			*handle = ACPI_PTR_INC_BY(
				*handle, ((acpiR::srat::sMem *)*handle)->length);

			break;
		};
	};

	return NULL;
}

