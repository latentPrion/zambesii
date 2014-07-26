
#include <__kclasses/debugPipe.h>
#include <commonlibs/libx86mp/ioApic.h>
#include <commonlibs/libx86mp/lapic.h>


error_t x86IoApic::ioApicC::get__kpinFor(uarch_t girqNo, ubit16 *__kpin)
{
	/**	EXPLANATION:
	 * We assume that any caller of this function is calling to ask for
	 * a lookup based on an ACPI ID. So we check to see if any of the pins
	 * on this IO-APIC match, and if not, we return an error.
	 *
	 * Naturally, if we are not aware of the ACPI global IRQ base for this
	 * IO-APIC, we cannot do any lookups at all.
	 **/
	if (acpiGirqBase == IRQCTL_IRQPIN_ACPIID_INVALID) {
		return ERROR_INVALID_ARG;
	};

	for (ubit8 i=0; i<nPins; i++)
	{
		if (irqPinList[i].acpiId == (sarch_t)girqNo)
		{
			*__kpin = irqPinList[i].__kid;
			return ERROR_SUCCESS;
		};
	};

	return ERROR_INVALID_ARG_VAL;
}

status_t x86IoApic::ioApicC::setIrqStatus(
	uarch_t __kpin, cpu_t cpu, uarch_t vector, ubit8 enabled
	)
{
	error_t		err;
	ubit8		pin;
	ubit8		dummy, destMode, deliveryMode, triggerMode, polarity;
	sarch_t		isEnabled;

	err = lookupPinBy__kid(__kpin, &pin);
	if (err != ERROR_SUCCESS) { return IRQCTL_GETIRQSTATUS_INEXISTENT; };

	isEnabled = getPinState(
		pin, &dummy, &dummy,
		&deliveryMode, &destMode, &polarity, &triggerMode);

	setPinState(
		pin, cpu, vector,
		deliveryMode, destMode, polarity, triggerMode);

	if (enabled && (!isEnabled))
	{
		unmaskPin(pin);
		FLAG_SET(irqPinList[pin].flags, IRQCTL_IRQPIN_FLAGS_ENABLED);
	};

	if ((!enabled) && isEnabled)
	{
		maskPin(pin);
		FLAG_UNSET(irqPinList[pin].flags, IRQCTL_IRQPIN_FLAGS_ENABLED);
	};

	irqPinList[pin].cpu = cpu;
	irqPinList[pin].vector = vector;

	return ERROR_SUCCESS;
}

status_t x86IoApic::ioApicC::getIrqStatus(
	uarch_t __kpin, cpu_t *cpu, uarch_t *vector,
	ubit8 *triggerMode, ubit8 *polarity
	)
{
	error_t		err;
	ubit8		pin;

	err = lookupPinBy__kid(__kpin, &pin);
	if (err != ERROR_SUCCESS) { return IRQCTL_GETIRQSTATUS_INEXISTENT; };

	*cpu = irqPinList[pin].cpu;
	*vector = irqPinList[pin].vector;
	*triggerMode = (irqPinList[pin].triggerMode == IRQCTL_IRQPIN_TRIGGMODE_EDGE)
		? x86IOAPIC_TRIGGMODE_EDGE : x86IOAPIC_TRIGGMODE_LEVEL;

	*polarity = (irqPinList[pin].triggerMode == IRQCTL_IRQPIN_POLARITY_LOW)
		? x86IOAPIC_POLARITY_LOW : x86IOAPIC_POLARITY_HIGH;

	return FLAG_TEST(irqPinList[pin].flags, IRQCTL_IRQPIN_FLAGS_ENABLED)
		? IRQCTL_GETIRQSTATUS_ENABLED : IRQCTL_GETIRQSTATUS_DISABLED;
}

void x86IoApic::ioApicC::maskIrq(ubit16 __kpin)
{
	error_t		err;
	ubit8		pin;

	err = lookupPinBy__kid(__kpin, &pin);
	if (err != ERROR_SUCCESS) { return; };

	maskPin(pin);
	FLAG_UNSET(irqPinList[pin].flags, IRQCTL_IRQPIN_FLAGS_ENABLED);
}

void x86IoApic::ioApicC::unmaskIrq(ubit16 __kpin)
{
	error_t		err;
	ubit8		pin;

	err = lookupPinBy__kid(__kpin, &pin);
	if (err != ERROR_SUCCESS) { return; };

	unmaskPin(pin);
	FLAG_SET(irqPinList[pin].flags, IRQCTL_IRQPIN_FLAGS_ENABLED);
}

sarch_t x86IoApic::ioApicC::irqIsEnabled(ubit16 __kpin)
{
	error_t		err;
	ubit8		pin;

	err = lookupPinBy__kid(__kpin, &pin);
	if (err != ERROR_SUCCESS) { return 0; };

	return FLAG_TEST(irqPinList[pin].flags, IRQCTL_IRQPIN_FLAGS_ENABLED);
}

void x86IoApic::ioApicC::maskAll(void)
{
	for (ubit8 i=0; i<nPins; i++)
	{
		maskPin(i);
		FLAG_UNSET(irqPinList[i].flags, IRQCTL_IRQPIN_FLAGS_ENABLED);
	}
}

void x86IoApic::ioApicC::unmaskAll(void)
{
	for (ubit8 i=0; i<nPins; i++)
	{
		unmaskPin(i);
		FLAG_SET(irqPinList[i].flags, IRQCTL_IRQPIN_FLAGS_ENABLED);
	}
}

