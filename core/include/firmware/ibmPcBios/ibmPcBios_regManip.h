#ifndef _IBM_PC_BIOS_REG_STATE_MANIP_H
	#define _IBM_PC_BIOS_REG_STATE_MANIP_H

	#include <__kstdlib/__ktypes.h>
	#include "x86Emu.h"

namespace ibmPcBios_regs
{
	uarch_t getEax(void);
	uarch_t getEbx(void);
	uarch_t getEcx(void);
	uarch_t getEdx(void);
	uarch_t getEsi(void);
	uarch_t getEdi(void);
	uarch_t getEsp(void);
	uarch_t getEbp(void);
	uarch_t getEflags(void);
	uarch_t getCs(void);
	uarch_t getDs(void);
	uarch_t getEs(void);
	uarch_t getFs(void);
	uarch_t getGs(void);
	uarch_t getSs(void);

	void setEax(uarch_t val);
	void setEbx(uarch_t val);
	void setEcx(uarch_t val);
	void setEdx(uarch_t val);
	void setEsi(uarch_t val);
	void setEdi(uarch_t val);
	void setEsp(uarch_t val);
	void setEbp(uarch_t val);
	void setCs(uarch_t val);
	void setDs(uarch_t val);
	void setEs(uarch_t val);
	void setFs(uarch_t val);
	void setGs(uarch_t val);
	void setSs(uarch_t val);
}

#endif

