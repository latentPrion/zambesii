
#include <firmware/ibmPcBios/ibmPcBios_regManip.h>

uarch_t ibmPcBios_regs::getEax(void)
{
	return M.x86.R_EAX;
}

uarch_t ibmPcBios_regs::getEbx(void)
{
	return M.x86.R_EBX;
}

uarch_t ibmPcBios_regs::getEcx(void)
{
	return M.x86.R_ECX;
}

uarch_t ibmPcBios_regs::getEdx(void)
{
	return M.x86.R_EDX;
}

uarch_t ibmPcBios_regs::getEsi(void)
{
	return M.x86.R_ESI;
}

uarch_t ibmPcBios_regs::getEdi(void)
{
	return M.x86.R_EDI;
}

uarch_t ibmPcBios_regs::getEsp(void)
{
	return M.x86.R_ESP;
}

uarch_t ibmPcBios_regs::getEbp(void)
{
	return M.x86.R_EBP;
}

uarch_t ibmPcBios_regs::getCs(void)
{
	return M.x86.R_CS;
}

uarch_t ibmPcBios_regs::getDs(void)
{
	return M.x86.R_DS;
}

uarch_t ibmPcBios_regs::getEs(void)
{
	return M.x86.R_ES;
}

uarch_t ibmPcBios_regs::getFs(void)
{
	return M.x86.R_FS;
}

uarch_t ibmPcBios_regs::getGs(void)
{
	return M.x86.R_GS;
}

uarch_t ibmPcBios_regs::getSs(void)
{
	return M.x86.R_SS;
}

uarch_t ibmPcBios_regs::getEflags(void)
{
	return M.x86.R_EFLG;
}

