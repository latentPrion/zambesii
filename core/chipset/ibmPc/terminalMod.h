#ifndef _IBM_PC_TERMINAL_MOD_H
	#define _IBM_PC_TERMINAL_MOD_H

	#include <chipset/pkg/debugMod.h>

#ifdef __cplusplus
#define IPTMEXTERN		extern "C"
#else
#define IPTMEXTERN		extern
#endif

IPTMEXTERN error_t ibmPc_terminalMod_initialize(void);
IPTMEXTERN error_t ibmPc_terminalMod_shutdown(void);
IPTMEXTERN error_t ibmPc_terminalMod_suspend(void);
IPTMEXTERN error_t ibmPc_terminalMod_awake(void);

IPTMEXTERN sarch_t ibmPc_terminalMod_isInitialized(void);
IPTMEXTERN void ibmPc_terminalMod_syphon(const utf8Char *str, uarch_t len);
IPTMEXTERN void ibmPc_terminalMod_clear(void);

IPTMEXTERN struct debugModS	ibmPc_terminalMod;

#undef IPTMEXTERN

#endif

