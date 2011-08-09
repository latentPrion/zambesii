
#include <commonlibs/libx86mp/lapic.h>


x86Lapic::sendPhysicalIpi(ubit8 type, ubit8 vector, ubit8 dest)
{
	// For SIPI, the vector field is ignored.
}
