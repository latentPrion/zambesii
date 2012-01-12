#ifndef _IBM_PC_PIC_H
	#define _IBM_PC_PIC_H

	#include <__kstdlib/__ktypes.h>
	#include <chipset/pkg/intControllerMod.h>

#ifdef __cplusplus
extern "C" {
#endif

error_t ibmPc_pic_initialize(void);
error_t ibmPc_pic_shutdown(void);
error_t ibmPc_pic_suspend(void);
error_t ibmPc_pic_restore(void);

void ibmPc_pic_maskAll(void);
error_t ibmPc_pic_maskSingle(uarch_t vector);
void ibmPc_pic_unmaskAll(void);
error_t ibmPc_pic_unmaskSingle(uarch_t vector);
void ibmPc_pic_sendEoi(ubit8);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" struct intControllerModS	ibmPc_intControllerMod;
#else
struct intControllerModS	ibmPc_intControllerMod;
#endif

#endif

