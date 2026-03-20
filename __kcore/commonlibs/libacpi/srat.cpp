
#include <__kclasses/debugPipe.h>
#include <commonlibs/libacpi/srat.h>

namespace
{
	struct sSratEntryHeader
	{
		ubit8	type;
		ubit8	length;
	};

	static void *getNextEntryOfType(
		acpiR::sSrat *srat, void **const handle, ubit8 requestedType)
	{
		sSratEntryHeader	*entry;

		if (*handle == NULL) {
			*handle = ACPI_SRAT_GET_FIRST_ENTRY(srat);
		};

		while (*handle < ACPI_TABLE_GET_ENDADDR(srat))
		{
			entry = static_cast<sSratEntryHeader *>( *handle );
			if (entry->length == 0) {
				return NULL;
			};

			switch (entry->type)
			{
			case ACPI_SRAT_TYPE_CPU:
			case ACPI_SRAT_TYPE_MEM:
				if (entry->type == requestedType)
				{
					void *ret = *handle;

					*handle = ACPI_PTR_INC_BY(*handle, entry->length);
					return ret;
				};

				break;

			default:
				break;
			};

			*handle = ACPI_PTR_INC_BY(*handle, entry->length);
		};

		return NULL;
	}
}


acpiR::srat::sCpu *acpiR::srat::getNextCpuEntry(
	acpiR::sSrat *srat, void **const handle
	)
{
	return static_cast<acpiR::srat::sCpu *>(
		getNextEntryOfType(srat, handle, ACPI_SRAT_TYPE_CPU));
}

acpiR::srat::sMem *acpiR::srat::getNextMemEntry(
	acpiR::sSrat *srat, void **const handle
	)
{
	return static_cast<acpiR::srat::sMem *>(
		getNextEntryOfType(srat, handle, ACPI_SRAT_TYPE_MEM));
}
