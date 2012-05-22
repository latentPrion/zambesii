#ifndef _IBM_PC_i8259a_H
	#define _IBM_PC_i8259a_H

	#include <chipset/zkcm/irqControl.h>
	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/smpTypes.h>

#define i8259a			"IBMPC-8259: "

#ifdef __cplusplus
extern "C" {
#endif

error_t ibmPc_i8259a_initialize(void);
error_t ibmPc_i8259a_shutdown(void);
error_t ibmPc_i8259a_suspend(void);
error_t ibmPc_i8259a_restore(void);

void ibmPc_i8259a_chipsetEventNotification(ubit8 event, uarch_t flags);

error_t ibmPc_i8259a_identifyIrq(uarch_t physicalId, ubit16 *__kpin);
status_t ibmPc_i8259a_getIrqStatus(
	uarch_t __kpin, cpu_t *cpu, uarch_t *vector,
	ubit8 *triggerMode, ubit8 *polarity);

status_t ibmPc_i8259a_setIrqStatus(
	uarch_t __kpin, cpu_t cpu, uarch_t vector, ubit8 enabled);

void ibmPc_i8259a_maskIrq(ubit16 __kid);
void ibmPc_i8259a_unmaskIrq(ubit16 __kid);
void ibmPc_i8259a_maskAll(void);
void ibmPc_i8259a_unmaskAll(void);

sarch_t ibmPc_i8259a_irqIsEnabled(ubit16 __kpin);

void ibmPc_i8259a_maskIrqsByPriority(
	ubit16 __kid, cpu_t cpuId, uarch_t *mask);

void ibmPc_i8259a_unmaskIrqsByPriority(
	ubit16 __kid, cpu_t cpuId, uarch_t mask);

void ibmPc_i8259a_sendEoi(ubit16 __kpin);

#ifdef __cplusplus
}
#endif

#endif

