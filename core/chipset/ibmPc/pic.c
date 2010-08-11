
#include <kernel/common/firmwareTrib/rivIoApi.h>
#include "pic.h"

#define PIC_PIC1_CMD
#define PIC_PIC1_DATA
#define PIC_PIC2_CMD
#define PIC_PIC2_DATA

void ibmPc_pic_remapIsaIrqs(void);

error_t ibmPc_pic_maskSingle(uarch_t vector)
{
	
error_t ibmPc_pic_maskAll(void);
error_t ibmPc_pic_unmaskSingle(uarch_t vector);
error_t ibmPc_pic_unmaskAll(void);

