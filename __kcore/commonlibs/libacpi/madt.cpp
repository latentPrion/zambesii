
#include <commonlibs/libacpi/madt.h>

namespace
{
	struct sMadtEntryHeader
	{
		ubit8	type;
		ubit8	length;
	};

	static void *getNextEntryOfType(
		acpiR::sMadt *madt, void **const handle, ubit8 requestedType)
	{
		sMadtEntryHeader	*entry;

		if (*handle == NULL) {
			*handle = ACPI_MADT_GET_FIRST_ENTRY(madt);
		};

		while (*handle < ACPI_TABLE_GET_ENDADDR(madt))
		{
			entry = static_cast<sMadtEntryHeader *>( *handle );
			if (entry->length == 0) {
				return NULL;
			};

			switch (entry->type)
			{
			case ACPI_MADT_TYPE_LAPIC:
			case ACPI_MADT_TYPE_IOAPIC:
			case ACPI_MADT_TYPE_IRQSOURCE_OVERRIDE:
			case ACPI_MADT_TYPE_NMI:
			case ACPI_MADT_TYPE_LAPIC_NMI:
			case ACPI_MADT_TYPE_LAPICADDR_OVERRIDE:
			case ACPI_MADT_TYPE_IOSAPIC:
			case ACPI_MADT_TYPE_LSAPIC:
			case ACPI_MADT_TYPE_IRQSOURCE:
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


acpiR::madt::sCpu *acpiR::madt::getNextCpuEntry(
	acpiR::sMadt *madt, void **const handle
	)
{
	return static_cast<acpiR::madt::sCpu *>(
		getNextEntryOfType(madt, handle, ACPI_MADT_TYPE_LAPIC));
}

acpiR::madt::sIoApic *acpiR::madt::getNextIoApicEntry(
	acpiR::sMadt *madt, void **const handle
	)
{
	return static_cast<acpiR::madt::sIoApic *>(
		getNextEntryOfType(madt, handle, ACPI_MADT_TYPE_IOAPIC));
}

acpiR::madt::sLapicNmi *acpiR::madt::getNextLapicNmiEntry(
	acpiR::sMadt *madt, void **const handle
	)
{
	return static_cast<acpiR::madt::sLapicNmi *>(
		getNextEntryOfType(madt, handle, ACPI_MADT_TYPE_LAPIC_NMI));
}

acpiR::madt::sIrqSourceOver *acpiR::madt::getNextIrqSourceOverrideEntry(
	acpiR::sMadt *madt, void **const handle
	)
{
	return static_cast<acpiR::madt::sIrqSourceOver *>(
		getNextEntryOfType(
			madt, handle, ACPI_MADT_TYPE_IRQSOURCE_OVERRIDE));
}
