
#include "ibmPcBios_regManip.h"

uarch_t ibmPcBios_getEax(void)
{
	return M.x86.R_EAX;
}

uarch_t ibmPcBios_getEbx(void)
{
	return M.x86.R_EBX;
}

uarch_t ibmPcBios_getEcx(void)
{
	return M.x86.R_ECX;
}

uarch_t ibmPcBios_getEdx(void)
{
	return M.x86.R_EDX;
}

uarch_t ibmPcBios_getEsi(void)
{
	return M.x86.R_ESI;
}

uarch_t ibmPcBios_getEdi(void)
{
	return M.x86.R_EDI;
}

uarch_t ibmPcBios_getEsp(void)
{
	return M.x86.R_ESP;
}

uarch_t ibmPcBios_getEbp(void)
{
	return M.x86.R_EBP;
}

uarch_t ibmPcBios_getCs(void)
{
	return M.x86.R_CS;
}

uarch_t ibmPcBios_getDs(void)
{
	return M.x86.R_DS;
}

uarch_t ibmPcBios_getEs(void)
{
	return M.x86.R_ES;
}

uarch_t ibmPcBios_getFs(void)
{
	return M.x86.R_FS;
}

uarch_t ibmPcBios_getGs(void)
{
	return M.x86.R_GS;
}

uarch_t ibmPcBios_getSs(void)
{
	return M.x86.R_SS;
}

uarch_t ibmPcBios_getEflags(void)
{
	return M.x86.R_EFLG;
}

