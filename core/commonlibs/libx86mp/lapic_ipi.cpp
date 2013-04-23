
#include <arch/interrupts.h>
#include <arch/cpuControl.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kclasses/debugPipe.h>
#include <commonlibs/libx86mp/libx86mp.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/interruptTrib/interruptTrib.h>


sarch_t		x86LapicC::ipiS::handlerIsInstalled=0;

error_t x86LapicC::ipiS::sendPhysicalIpi(
	ubit8 type, ubit8 vector, ubit8 shortDest, cpu_t dest
	)
{
	// Timeout value is the one used by Linux.
	ubit32		timeout=1000;
	x86LapicC	*lapic;

	lapic = &cpuTrib.getCurrentCpuStream()->lapic;

	// For SIPI, the vector field is ignored.
	if (shortDest == x86LAPIC_IPI_SHORTDEST_NONE) {
		lapic->write32(x86LAPIC_REG_INT_CMD1, dest << 24);
	};

	lapic->write32(
		x86LAPIC_REG_INT_CMD0,
		vector
		| (type << x86LAPIC_IPI_TYPE_SHIFT)
		| (x86LAPIC_IPI_DESTMODE_PHYS << x86LAPIC_IPI_DESTMODE_SHIFT)
		| (((type == x86LAPIC_IPI_TYPE_ARBIT_RESET) ?
			x86LAPIC_IPI_LEVEL_ARBIT_RESET
			: x86LAPIC_IPI_LEVEL_OTHER)
			<< x86LAPIC_IPI_LEVEL_SHIFT)
		| (x86LAPIC_IPI_TRIGG_EDGE << x86LAPIC_IPI_TRIGG_SHIFT)
		| (shortDest << x86LAPIC_IPI_SHORTDEST_SHIFT));

	while (__KFLAG_TEST(
		lapic->read32(x86LAPIC_REG_INT_CMD0),
		x86LAPIC_IPI_DELIVERY_STATUS_PENDING) && timeout != 0)
	{
		timeout--;
		cpuControl::subZero();
	};

	// Check for successful delivery here.
	return (timeout != 0) ? ERROR_SUCCESS : ERROR_UNKNOWN;
}

error_t x86LapicC::ipiS::setupIpis(cpuStreamC *)
{
	installHandler();
	return ERROR_SUCCESS;
}

void x86LapicC::ipiS::installHandler(void)
{
	if (handlerIsInstalled) { return; };

	// Ask the Interrupt Trib to take our handler.
	interruptTrib.installException(
		x86LAPIC_VECTOR_IPI,
		(__kexceptionFn *)&x86LapicC::ipiS::exceptionHandler,
		INTTRIB_EXCEPTION_FLAGS_POSTCALL);

	handlerIsInstalled = 1;
}

status_t x86LapicC::ipiS::exceptionHandler(taskContextC *, ubit8 postcall)
{
	// Check messager and see why we got an IPI.
	if (!postcall)
	{
		cpuTrib.getCurrentCpuStream()->interCpuMessager.dispatch();
	};

	cpuTrib.getCurrentCpuStream()->lapic.sendEoi();
	return ERROR_SUCCESS;
}

