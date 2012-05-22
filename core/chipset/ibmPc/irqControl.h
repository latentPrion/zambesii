#ifndef _IBM_PC_IRQ_CONTROL_H
	#define _IBM_PC_IRQ_CONTROL_H

	#include <extern.h>
	#include <chipset/zkcm/irqControl.h>
	#include <__kstdlib/__ktypes.h>

#define IBMPCIRQCTL		"IBMPC Irq-Ctl: "

CPPEXTERN_START

error_t ibmPc_irqControl_initialize(void);
error_t ibmPc_irqControl_shutdown(void);
error_t ibmPc_irqControl_suspend(void);
error_t ibmPc_irqControl_restore(void);

error_t ibmPc_irqControl_registerIrqController(void);
void ibmPc_irqControl_destroyIrqController(void);

void ibmPc_irqControl_chipsetEventNotification(ubit8 event, uarch_t flags);

error_t ibmPc_irqControl_identifyIrq(uarch_t physicalId, ubit16 *__kpin);
status_t ibmPc_irqControl_getIrqStatus(
	uarch_t __kpin, cpu_t *cpu, uarch_t *vector,
	ubit8 *triggerMode, ubit8 *polarity);

status_t ibmPc_irqControl_setIrqStatus(
	uarch_t __kpin, cpu_t cpu, uarch_t vector, ubit8 enabled);

void ibmPc_irqControl_maskIrq(ubit16 __kid);
void ibmPc_irqControl_unmaskIrq(ubit16 __kid);
void ibmPc_irqControl_maskAll(void);
void ibmPc_irqControl_unmaskAll(void);

sarch_t ibmPc_irqControl_irqIsEnabled(ubit16 __kpin);

void ibmPc_irqControl_maskIrqsByPriority(
	ubit16 __kid, cpu_t cpuId, uarch_t *mask);

void ibmPc_irqControl_unmaskIrqsByPriority(
	ubit16 __kid, cpu_t cpuId, uarch_t mask);

void ibmPc_irqControl_sendEoi(ubit16 __kpin);

CPPEXTERN_END

CPPEXTERN_PROTO struct zkcmIrqControlModS		ibmPc_irqControlMod;

#endif

