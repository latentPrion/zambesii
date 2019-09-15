#ifndef _ZK_CHIPSET_MODULE_CORE_H
	#define _ZK_CHIPSET_MODULE_CORE_H

	#include <__kstdlib/__ktypes.h>
	#include <chipset/zkcm/memoryDetection.h>
	#include <chipset/zkcm/cpuDetection.h>
	#include <chipset/zkcm/irqControl.h>
	#include <chipset/zkcm/timerControl.h>
	#include <chipset/zkcm/debugDevice.h>
	#include <chipset/pkg/watchdogMod.h>
	#include <kernel/common/smpTypes.h>

#define ZKCMCORE	"ZKCM Core: "

class ZkcmCore
{
public:
	ZkcmCore(utf8Char *chipsetName, utf8Char *chipsetVendor);

public:
	error_t initialize(void);
	error_t shutdown(void);
	error_t suspend(void);
	error_t restore(void);

	utf8Char	chipsetName[96];
	utf8Char	chipsetVendor[96];

	ZkcmMemoryDetectionMod		memoryDetection;
	ZkcmCpuDetectionMod		cpuDetection;

	// XXX: Still to be updated.
	WatchdogMod			watchdog;
	ZkcmIrqControlMod		irqControl;
	ZkcmTimerControlMod		timerControl;

	ZkcmDebugDevice		*debug[4];

	void chipsetEventNotification(e__kPowerEvent event, uarch_t flags);
	void newCpuIdNotification(cpu_t highestCpuId);
};

extern ZkcmCore		zkcmCore;

#endif

