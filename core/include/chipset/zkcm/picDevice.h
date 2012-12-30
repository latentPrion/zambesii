#ifndef _ZKCM_PIC_DEVICE_H
	#define _ZKCM_PIC_DEVICE_H

	#include <chipset/zkcm/device.h>
	#include <chipset/zkcm/irqControl.h>
	#include <__kstdlib/__ktypes.h>

class zkcmPicDeviceC
:
public zkcmDeviceBaseC
{
public:
	zkcmPicDeviceC(ubit16 nPins, zkcmDeviceC *device)
	:
	zkcmDeviceBaseC(device),
	nPins(nPins), irqPinList(__KNULL),
	__kpinBase(0)
	{}

public:
	virtual error_t initialize(void)=0;
	virtual error_t shutdown(void)=0;
	virtual error_t suspend(void)=0;
	virtual error_t restore(void)=0;

	virtual ubit16 get__kpinBase(void)
	{
		return __kpinBase;
	}

	virtual error_t identifyActiveIrq(
		cpu_t cpu, uarch_t vector, ubit16 *__kpin, ubit8 *triggerMode);

	virtual status_t getIrqStatus(
		uarch_t __kpin, cpu_t *cpu, uarch_t *vector,
		ubit8 *triggerMode, ubit8 *polarity)=0;

	virtual status_t setIrqStatus(
		uarch_t __kpin, cpu_t cpu, uarch_t vector, ubit8 enabled)=0;

	virtual void maskIrq(ubit16 __kpin)=0;
	virtual void unmaskIrq(ubit16 __kpin)=0;
	virtual void maskAll(void)=0;
	virtual void unmaskAll(void)=0;
	virtual sarch_t irqIsEnabled(ubit16 __kpin)=0;

	virtual void maskIrqsByPriority(
		ubit16 __kpin, cpu_t cpuId, uarch_t *mask0)=0;

	virtual void unmaskIrqsByPriority(
		ubit16 __kpin, cpu_t cpuId, uarch_t mask0)=0;

	virtual void sendEoi(ubit16 __kpin)=0;

public:
	ubit16		nPins;
	zkcmIrqPinS	*irqPinList;
	ubit16		__kpinBase;
};

#endif

