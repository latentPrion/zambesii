#ifndef _ZK_CHIPSET_MODULE_CORE_H
	#define _ZK_CHIPSET_MODULE_CORE_H

	#include <__kstdlib/__ktypes.h>
	#include <chipset/zkcm/memoryDetection.h>
	#include <chipset/zkcm/cpuDetection.h>
	#include <chipset/zkcm/irqControl.h>
	#include <chipset/zkcm/timerControl.h>
	#include <chipset/zkcm/debugDevice.h>
	#include <chipset/pkg/watchdogMod.h>

class zkcmCoreC
{
public:
	zkcmCoreC(utf8Char *chipsetName, utf8Char *chipsetVendor);

public:
	error_t initialize(void);
	error_t shutdown(void);
	error_t suspend(void);
	error_t restore(void);

	utf8Char	chipsetName[96];
	utf8Char	chipsetVendor[96];

	zkcmMemoryDetectionModC		memoryDetection;
	zkcmCpuDetectionModC		cpuDetection;

	// XXX: Still to be updated.
	watchdogModS			watchdog;
	zkcmIrqControlModC		irqControl;
	zkcmTimerControlModC		timerControl;

	zkcmDebugDeviceC		*debug[4];
};

extern zkcmCoreC		zkcmCore;

#endif

