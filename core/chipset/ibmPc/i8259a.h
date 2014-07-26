#ifndef _IBM_PC_i8259a_H
	#define _IBM_PC_i8259a_H

	#include <chipset/zkcm/picDevice.h>
	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/smpTypes.h>

#define i8259a			"IBMPC-8259: "

class i8259aPicC
:
public ZkcmPicDevice
{
public:
	i8259aPicC(ubit16 nPins)
	:
	ZkcmPicDevice(nPins, &baseDeviceInfo),
	baseDeviceInfo(
		0, CC"i8259a", CC"IBM-PC Compatible i8259a PIC chain",
		CC"Unknown vendor", CC"N/A")
	{}

public:
	virtual error_t initialize(void);
	virtual error_t shutdown(void);
	virtual error_t suspend(void);
	virtual error_t restore(void);

	virtual error_t identifyActiveIrq(
		cpu_t cpu, uarch_t vector, ubit16 *__kpin, ubit8 *triggerMode);

	virtual status_t getIrqStatus(
		uarch_t __kpin, cpu_t *cpu, uarch_t *vector,
		ubit8 *triggerMode, ubit8 *polarity);

	virtual status_t setIrqStatus(
		uarch_t __kpin, cpu_t cpu, uarch_t vector, ubit8 enabled);

	virtual void maskIrq(ubit16 __kpin);
	virtual void unmaskIrq(ubit16 __kpin);
	virtual void maskAll(void);
	virtual void unmaskAll(void);
	virtual sarch_t irqIsEnabled(ubit16 __kpin);

	virtual void maskIrqsByPriority(
		ubit16 __kpin, cpu_t cpuId, uarch_t *mask0);

	virtual void unmaskIrqsByPriority(
		ubit16 __kpin, cpu_t cpuId, uarch_t mask0);

	virtual void sendEoi(ubit16 __kpin);

public:
	error_t get__kpinFor(ubit8 pinNo, ubit16 *__kpin);
	error_t lookupPinBy__kid(ubit16 __kpin, ubit8 *pin);
	void chipsetEventNotification(ubit8 event, uarch_t);

private:
	ZkcmDevice	baseDeviceInfo;
};

extern i8259aPicC	i8259aPic;


/**	Inline methods.
 ******************************************************************************/

inline error_t i8259aPicC::lookupPinBy__kid(ubit16 __kpin, ubit8 *pin)
{
	*pin = __kpin - __kpinBase;
	if (*pin > 15) { return ERROR_INVALID_ARG_VAL; };
	return ERROR_SUCCESS;
}

#endif

