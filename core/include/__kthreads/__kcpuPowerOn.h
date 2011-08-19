#ifndef ___K_CPU_POWER_ON_H
	#define ___K_CPU_POWER_ON_H

extern "C" void __kcpuPowerOnEntry(void);
extern "C" void __kcpuPowerOnHll(void);
void __kcpuPowerOnMain(void);

// Points to the waking CPU's CPU stream.
extern "C" cpuStreamC	*__kcpuPowerOnSelf;

#endif

