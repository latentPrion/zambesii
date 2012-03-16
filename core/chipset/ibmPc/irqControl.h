#ifndef _IBM_PC_IRQ_CONTROL_H
	#define _IBM_PC_IRQ_CONTROL_H

	#include <extern.h>
	#include <chipset/zkcm/irqControl.h>
	#include <__kstdlib/__ktypes.h>

#define IBMPCIRQCTL		"IBMPC Irq-Ctl: "

CPPEXTERN_START

error_t ibmPc_irqControl_initialize(void);
error_t ibmPc_irqControl_detectPins(ubit16 *nPins, struct zkcmIrqPinS **ret);
error_t ibmPc_irqControl_shutdown(void);
error_t ibmPc_irqControl_suspend(void);
error_t ibmPc_irqControl_restore(void);

void ibmPc_irqControl___kregisterPinIds(struct zkcmIrqPinS *list);

void ibmPc_irqControl_maskIrq(ubit16 __kid);
void ibmPc_irqControl_unmaskIrq(ubit16 __kid);
void ibmPc_irqControl_maskAll(void);
void ibmPc_irqControl_unmaskAll(void);

void ibmPc_irqControl_maskIrqsByPriority(
	ubit16 __kid, cpu_t cpuId, uarch_t *mask);

void ibmPc_irqControl_unmaskIrqsByPriority(
	ubit16 __kid, cpu_t cpuId, uarch_t mask);

sarch_t ibmPc_irqControl_getIrqStatus(ubit16 __kid);

CPPEXTERN_END

CPPEXTERN_PROTO struct zkcmIrqControlModS		ibmPc_irqControlMod;

#endif

