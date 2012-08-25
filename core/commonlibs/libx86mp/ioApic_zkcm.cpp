
#include <__kclasses/debugPipe.h>
#include <commonlibs/libx86mp/ioApic.h>
#include <commonlibs/libx86mp/lapic.h>


error_t x86IoApic::ioApicC::identifyIrq(uarch_t physicalId, ubit16 *__kpin)
{
	/**	EXPLANATION:
	 * We assume that any caller of this function is calling to ask for
	 * a lookup based on an ACPI ID. So we check to see if any of the pins
	 * on this IO-APIC match, and if not, we return an error.
	 *
	 * Naturally, if we are not aware of the ACPI global IRQ base for this
	 * IO-APIC, we cannot do any lookups at all.
	 **/
	if (acpiGirqBase == IRQPIN_ACPIID_INVALID) {
		return ERROR_INVALID_ARG;
	};

	for (ubit8 i=0; i<nIrqs; i++)
	{
		if (irqPinList[i].acpiId == (sarch_t)physicalId)
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
	if (err != ERROR_SUCCESS) { return IRQCTL_IRQSTATUS_INEXISTENT; };

	isEnabled = getPinState(
		pin, &dummy, &dummy,
		&deliveryMode, &destMode, &polarity, &triggerMode);

	setPinState(
		pin, cpu, vector,
		deliveryMode, destMode, polarity, triggerMode);

	if (enabled && (!isEnabled))
	{
		unmaskPin(pin);
		__KFLAG_SET(irqPinList[pin].flags, IRQPIN_FLAGS_ENABLED);
	};

	if ((!enabled) && isEnabled)
	{
		maskPin(pin);
		__KFLAG_UNSET(irqPinList[pin].flags, IRQPIN_FLAGS_ENABLED);
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
	if (err != ERROR_SUCCESS) { return IRQCTL_IRQSTATUS_INEXISTENT; };

	*cpu = irqPinList[pin].cpu;
	*vector = irqPinList[pin].vector;
	*triggerMode = (irqPinList[pin].triggerMode == IRQPIN_TRIGGMODE_EDGE)
		? x86IOAPIC_TRIGGMODE_EDGE : x86IOAPIC_TRIGGMODE_LEVEL;

	*polarity = (irqPinList[pin].triggerMode == IRQPIN_POLARITY_LOW)
		? x86IOAPIC_POLARITY_LOW : x86IOAPIC_POLARITY_HIGH;

	return __KFLAG_TEST(irqPinList[pin].flags, IRQPIN_FLAGS_ENABLED) ?
		IRQCTL_IRQSTATUS_ENABLED : IRQCTL_IRQSTATUS_DISABLED;
}

void x86IoApic::ioApicC::maskIrq(ubit16 __kpin)
{
	error_t		err;
	ubit8		pin;

	err = lookupPinBy__kid(__kpin, &pin);
	if (err != ERROR_SUCCESS) { return; };

	maskPin(pin);
	__KFLAG_UNSET(irqPinList[pin].flags, IRQPIN_FLAGS_ENABLED);
}

void x86IoApic::ioApicC::unmaskIrq(ubit16 __kpin)
{
	error_t		err;
	ubit8		pin;

	err = lookupPinBy__kid(__kpin, &pin);
	if (err != ERROR_SUCCESS) { return; };

	unmaskPin(pin);
	__KFLAG_SET(irqPinList[pin].flags, IRQPIN_FLAGS_ENABLED);
}

sarch_t x86IoApic::ioApicC::irqIsEnabled(ubit16 __kpin)
{
	error_t		err;
	ubit8		pin;

	err = lookupPinBy__kid(__kpin, &pin);
	if (err != ERROR_SUCCESS) { return 0; };

	return __KFLAG_TEST(irqPinList[pin].flags, IRQPIN_FLAGS_ENABLED);
}

void x86IoApic::ioApicC::maskAll(void)
{
	for (ubit8 i=0; i<nIrqs; i++)
	{
		maskPin(i);
		__KFLAG_UNSET(irqPinList[i].flags, IRQPIN_FLAGS_ENABLED);
	}
}

void x86IoApic::ioApicC::unmaskAll(void)
{
	for (ubit8 i=0; i<nIrqs; i++)
	{
		unmaskPin(i);
		__KFLAG_SET(irqPinList[i].flags, IRQPIN_FLAGS_ENABLED);
	}
}

