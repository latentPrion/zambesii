
#include <firmware/ibmPcBios/ibmPcBios_regManip.h>

void ibmPcBios_regs::setEax(uarch_t val)
{
	M.x86.R_EAX = val;
}

void ibmPcBios_regs::setEbx(uarch_t val)
{
	M.x86.R_EBX = val;
}

void ibmPcBios_regs::setEcx(uarch_t val)
{
	M.x86.R_ECX = val;
}

void ibmPcBios_regs::setEdx(uarch_t val)
{
	M.x86.R_EDX = val;
}

void ibmPcBios_regs::setEsi(uarch_t val)
{
	M.x86.R_ESI = val;
}

void ibmPcBios_regs::setEdi(uarch_t val)
{
	M.x86.R_EDI = val;
}

void ibmPcBios_regs::setEsp(uarch_t val)
{
	M.x86.R_ESP = val;
}

void ibmPcBios_regs::setEbp(uarch_t val)
{
	M.x86.R_EBP = val;
}

void ibmPcBios_regs::setCs(uarch_t val)
{
	M.x86.R_CS = val;
}

void ibmPcBios_regs::setDs(uarch_t val)
{
	M.x86.R_DS = val;
}

void ibmPcBios_regs::setEs(uarch_t val)
{
	M.x86.R_ES = val;
}

void ibmPcBios_regs::setFs(uarch_t val)
{
	M.x86.R_FS = val;
}

void ibmPcBios_regs::setGs(uarch_t val)
{
	M.x86.R_GS = val;
}

void ibmPcBios_regs::setSs(uarch_t val)
{
	M.x86.R_SS = val;
}


