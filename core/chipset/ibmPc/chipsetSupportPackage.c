
#include <kernel/common/moduleApis/chipsetSupportPackage.h>

extern struct intControllerDevS		ibmPc_8259a;
extern struct continuousTimerDevS	ibmPc_pit;

static struct intControllerDevS *ibmPc_getIntController(
	struct intControllerDevS *i
	)
{
	(void)i;

	return &ibmPc_8259a;
}

static struct continuousTimerDevS *ibmPc_getSchedTimer(
	struct continuousTimerDevS *c
	)
{
	(void)c;

	return &ibmPc_pit;
}

struct chipsetSupportPackageS		chipsetCoreDev =
{
	__KNULL,
	&ibmPc_getIntController,
	&ibmPc_getSchedTimer
};

