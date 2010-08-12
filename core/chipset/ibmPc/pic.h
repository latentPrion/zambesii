#ifndef _IBM_PC_PIC_H
	#define _IBM_PC_PIC_H

	#include <__kstdlib/__ktypes.h>

#ifdef __cplusplus
extern "C" {
#endif

void ibmPc_pic_remapIsaIrqs(void);

error_t ibmPc_initializeInterrupts(void);
error_t ibmPC_initializeInterrupts2(void);

error_t ibmPc_pic_maskSingle(uarch_t vector);
error_t ibmPc_pic_maskAll(void);
error_t ibmPc_pic_unmaskSingle(uarch_t vector);
error_t ibmPc_pic_unmaskAll(void);

#ifdef __cplusplus
}
#endif

#endif

