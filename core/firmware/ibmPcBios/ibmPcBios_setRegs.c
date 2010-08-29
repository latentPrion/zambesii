
#include "ibmPcBios_regManip.h"

void ibmPcBios_setEax(uarch_t val)
{
	M.x86.R_EAX = val;
}

void ibmPcBios_setEbx(uarch_t val)
{
	M.x86.R_EBX = val;
}

void ibmPcBios_setEcx(uarch_t val)
{
	M.x86.R_ECX = val;
}

void ibmPcBios_setEdx(uarch_t val)
{
	M.x86.R_EDX = val;
}

void ibmPcBios_setEsi(uarch_t val)
{
	M.x86.R_ESI = val;
}

void ibmPcBios_setEdi(uarch_t val)
{
	M.x86.R_EDI = val;
}

void ibmPcBios_setEsp(uarch_t val)
{
	M.x86.R_ESP = val;
}

void ibmPcBios_setEbp(uarch_t val)
{
	M.x86.R_EBP = val;
}

void ibmPcBios_setCs(uarch_t val)
{
	M.x86.R_CS = val;
}

void ibmPcBios_setDs(uarch_t val)
{
	M.x86.R_DS = val;
}

void ibmPcBios_setEs(uarch_t val)
{
	M.x86.R_ES = val;
}

void ibmPcBios_setFs(uarch_t val)
{
	M.x86.R_FS = val;
}

void ibmPcBios_setGs(uarch_t val)
{
	M.x86.R_GS = val;
}

void ibmPcBios_setSs(uarch_t val)
{
	M.x86.R_SS = val;
}


