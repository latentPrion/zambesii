#ifndef _IBM_PC_BIOS_REG_STATE_MANIP_H
	#define _IBM_PC_BIOS_REG_STATE_MANIP_H

	#include <__kstdlib/__ktypes.h>
	#include "x86Emu.h"

#ifdef __cplusplus
extern "C" {
#endif

uarch_t ibmPcBios_getEax(void);
uarch_t ibmPcBios_getEbx(void);
uarch_t ibmPcBios_getEcx(void);
uarch_t ibmPcBios_getEdx(void);
uarch_t ibmPcBios_getEsi(void);
uarch_t ibmPcBios_getEdi(void);
uarch_t ibmPcBios_getEsp(void);
uarch_t ibmPcBios_getEbp(void);
uarch_t ibmPcBios_getCs(void);
uarch_t ibmPcBios_getDs(void);
uarch_t ibmPcBios_getEs(void);
uarch_t ibmPcBios_getFs(void);
uarch_t ibmPcBios_getGs(void);
uarch_t ibmPcBios_getSs(void);

void ibmPcBios_setEax(uarch_t val);
void ibmPcBios_setEbx(uarch_t val);
void ibmPcBios_setEcx(uarch_t val);
void ibmPcBios_setEdx(uarch_t val);
void ibmPcBios_setEsi(uarch_t val);
void ibmPcBios_setEdi(uarch_t val);
void ibmPcBios_setEsp(uarch_t val);
void ibmPcBios_setEbp(uarch_t val);
void ibmPcBios_setCs(uarch_t val);
void ibmPcBios_setDs(uarch_t val);
void ibmPcBios_setEs(uarch_t val);
void ibmPcBios_setFs(uarch_t val);
void ibmPcBios_setGs(uarch_t val);
void ibmPcBios_setSs(uarch_t val);

#ifdef __cplusplus
}
#endif

#endif

