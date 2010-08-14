
#include <kernel/common/moduleApis/chipsetSupportPackage.h>

extern struct intControllerDevS		ibmPc_8259a;

struct chipsetSupportPackageS		chipsetCoreDev =
{
	__KNULL,
	&ibmPc_8259a
};

