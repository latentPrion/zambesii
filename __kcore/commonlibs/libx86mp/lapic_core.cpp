
#include <arch/paddr_t.h>
#include <arch/walkerPageRanger.h>
#include <arch/x8632/cpuid.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kclib/assert.h>
#include <__kclasses/debugPipe.h>
#include <commonlibs/libx86mp/lapic.h>
#include <commonlibs/libx86mp/mpTables.h>
#include <commonlibs/libacpi/libacpi.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/processTrib/processTrib.h>
#include <__kthreads/__kcpuPowerOn.h>


#define x86LAPIC_NPAGES		4

X86Lapic::sCache		X86Lapic::cache;

X86Lapic::X86Lapic(CpuStream *parent)
:
ZkcmDevice(
	(ubit32)parent->cpuId,
	CC"Lapic", CC"Local Advanced Programmable IRQ Controller",
	CC"Unknown", CC"Unknown vendor"),
parent(parent)
/* Soon...
 *timer(
	ZkcmTimerDevice::CHIPSET,
	ZKCM_TIMERDEV_CAP_MODE_PERIODIC | ZKCM_TIMERDEV_CAP_MODE_ONESHOT,
	0, 0, 0, 0,
	ZkcmTimerDevice::LOW, ZkcmTimerDevice::HIGH,
	this)*/
{}

sarch_t X86Lapic::cpuHasLapic(void)
{
	uarch_t			eax, ebx, ecx, edx;

	assert_fatal(cpuTrib.getCurrentCpuStream()->cpuId == parent->cpuId);
	execCpuid(1, &eax, &ebx, &ecx, &edx);
	if (!(edx & (1 << 9)))
	{
		printf(ERROR x86LAPIC"%d: cpuHasLapic: CPUID[1].EDX[9] "
			"LAPIC check failed.\n\tEDX: %x.\n",
			parent->cpuId, edx);

		return 0;
	};

	return 1;
}

sarch_t X86Lapic::lapicMemIsMapped(void)
{
	if (cache.magic == x86LAPIC_MAGIC && cache.v != NULL) {
		return 1;
	};

	return 0;
}

error_t X86Lapic::mapLapicMem(void)
{
	paddr_t		p;
	void		*v;
	error_t		ret;

	if (lapicMemIsMapped()) { return ERROR_SUCCESS; };

	ret = detectPaddr();
	if (ret != ERROR_SUCCESS) { return ret; };
	if (!getPaddr(&p)) { return ERROR_UNKNOWN; };

	// Map the LAPIC paddr to the kernel's vaddrspace.
	v = walkerPageRanger::createMappingTo(
		p, x86LAPIC_NPAGES,
		PAGEATTRIB_PRESENT | PAGEATTRIB_WRITE | PAGEATTRIB_SUPERVISOR
		| PAGEATTRIB_CACHE_DISABLED);

	if (v == NULL)
	{
		printf(ERROR x86LAPIC"Failed to map LAPIC paddr.\n");
		return ERROR_MEMORY_VIRTUAL_PAGEMAP;
	};

	v = WPRANGER_ADJUST_VADDR(v, p, void *);
	cache.v = __kcpuPowerOnLapicVaddr = static_cast<ubit8 *>( v );
	return ERROR_SUCCESS;
}

error_t X86Lapic::detectPaddr(void)
{
	x86Mp::sConfig		*cfgTable;
	acpi::sRsdt		*rsdt;
	acpiR::sMadt		*madt;
	void			*handle, *context;
	paddr_t			tmp;

	if (getPaddr(&tmp)) { return ERROR_SUCCESS; };

	acpi::initializeCache();
	if (acpi::findRsdp() != ERROR_SUCCESS) { goto tryMpTables; };
#if !defined(__32_BIT__) || defined(CONFIG_ARCH_x86_32_PAE)
	// If not 32 bit, try XSDT.
	if (acpi::testForXsdt())
	{
		// Map XSDT, etc.
	}
	else // Continues into an else-if for RSDT test.
#endif
	// 32 bit uses RSDT only.
	if (acpi::testForRsdt())
	{
		if (acpi::mapRsdt() != ERROR_SUCCESS) { goto tryMpTables; };

		rsdt = acpi::getRsdt();
		context = handle = NULL;
		madt = acpiRsdt::getNextMadt(rsdt, &context, &handle);

		if (madt == NULL) { goto tryMpTables; };
		tmp = madt->lapicPaddr;

		acpiRsdt::destroyContext(&context);
		acpiRsdt::destroySdt(reinterpret_cast<acpi::sSdt *>( madt ));

		// Have the LAPIC paddr. Move on.
		goto initLibLapic;
	}
	else
	{
		printf(WARNING x86LAPIC"detectPaddr(): RSDP found, but no "
			"RSDT or XSDT.\n");
	};

tryMpTables:
	x86Mp::initializeCache();
	if (!x86Mp::mpConfigTableIsMapped())
	{
		x86Mp::findMpFp();
		if (!x86Mp::mpFpFound()) {
			goto useDefaultPaddr;
		};

		if (x86Mp::mapMpConfigTable() == NULL) {
			goto useDefaultPaddr;
		};
	};

	cfgTable = x86Mp::getMpCfg();
	tmp = cfgTable->lapicPaddr;
	goto initLibLapic;

useDefaultPaddr:
	printf(WARNING x86LAPIC"detectPaddr: Using default paddr.\n");
	tmp = 0xFEE00000;

initLibLapic:
	printf(NOTICE x86LAPIC"detectPaddr: LAPIC paddr: %P.\n",
		&tmp);

	X86Lapic::setPaddr(tmp);
	return ERROR_SUCCESS;
}

#define x86LAPIC_FLAG_SOFT_ENABLE		(1<<8)

void X86Lapic::softEnable(void)
{
	ubit32		outval;

	// Set the enabled flag in the spurious int vector reg.
	outval = read32(x86LAPIC_REG_SPURIOUS_VECT);
	FLAG_SET(outval, x86LAPIC_FLAG_SOFT_ENABLE);
	write32(x86LAPIC_REG_SPURIOUS_VECT, outval);
}

void X86Lapic::softDisable(void)
{
	ubit32		outval;

	outval = read32(x86LAPIC_REG_SPURIOUS_VECT);
	FLAG_UNSET(outval, x86LAPIC_FLAG_SOFT_ENABLE);
	write32(x86LAPIC_REG_SPURIOUS_VECT, outval);
}

sarch_t X86Lapic::isSoftEnabled(void)
{
	ubit32		regVal;

	regVal = read32(x86LAPIC_REG_SPURIOUS_VECT);
	return FLAG_TEST(regVal, x86LAPIC_FLAG_SOFT_ENABLE);
}

void X86Lapic::sendEoi(void)
{
	assert_fatal(cpuTrib.getCurrentCpuStream()->cpuId == parent->cpuId);
	write32(x86LAPIC_REG_EOI, 0);
}

#define x86LAPIC_LVT_ERR_FLAGS_DISABLE	(1<<16)

error_t X86Lapic::sError::setupLvtError(CpuStream *parent)
{
	parent->lapic.write32(
		x86LAPIC_REG_LVT_ERR, 0 | x86LAPIC_VECTOR_LVT_ERROR);

	installHandler();
	return ERROR_SUCCESS;
}

void X86Lapic::sError::installHandler(void)
{
	if (handlerIsInstalled) { return; };
}

#define x86LAPIC_SPURIOUS_VECTOR_FLAGS_DISABLE		(1<<8)

error_t X86Lapic::sSpurious::setupSpuriousVector(CpuStream *parent)
{
	ubit32		outval;

	outval = parent->lapic.read32(x86LAPIC_REG_SPURIOUS_VECT);
	parent->lapic.write32(
		x86LAPIC_REG_SPURIOUS_VECT, outval | x86LAPIC_VECTOR_SPURIOUS);

	return ERROR_SUCCESS;
}

void X86Lapic::sSpurious::installHandler(void)
{
	if (handlerIsInstalled) { return; };
}

ubit32 X86Lapic::read32(ubit32 offset)
{
	assert_fatal(cpuTrib.getCurrentCpuStream()->cpuId == parent->cpuId);
	return *reinterpret_cast<volatile ubit32*>( (uarch_t)cache.v + offset );
}

void X86Lapic::write32(ubit32 offset, ubit32 val)
{
	assert_fatal(cpuTrib.getCurrentCpuStream()->cpuId == parent->cpuId);
	// Linux uses XCHG to write to the LAPIC for certain hardware.
	*reinterpret_cast<ubit32 *>( (uarch_t)cache.v + offset ) = val;
}

#include <debug.h>
error_t X86Lapic::setupLapic(void)
{
	/**	EXPLANATION:
	 * Basically, ensure that the LAPIC is in a sane, predefined state
	 * so that it can handle interrupts, exceptions, IPIs, NMIs, and provide
	 * a scheduler timer.
	 *
	 * For the LAPIC's LINT inputs, we search BOTH tables (MP and ACPI)
	 * simply because I have found real hardware cases where there is an
	 * MP config table that says that there are extINT inputs on LAPICs.
	 * At the same time, ACPI doesn't give information about extINT, and
	 * only tells you about NMI. So if we just take ACPI only and ignore
	 * MP tables if ACPI was found, we may be missing out on the extINT
	 * programming information.
	 *
	 * Technically though, extINT doesn't matter when the chipset is in
	 * SMP mode: Symm. IO mode uses the IO APICs and not any extINT
	 * compatible PIC, so an extINT should never hit any CPU. Regardless,
	 * there's no harm in being thorough.
	 *
	 *	NOTE:
	 * Intel manual, Vol3A, Section 10.4.7.2: State of the LAPIC while it is
	 * temporarily disabled:
	 *	* LAPIC will respond normally to INIT, NMI, SMI and SIPI.
	 *	* Pending INTs will require handling and are held until cleared.
	 *	* The LAPIC can still be used safely to issue IPIs.
	 *	* Attempts to set or modify the LVT registers will be ignored.
	 *	* LAPIC continues to monitor the bus to keep synchronized.
	 *
	 * In particular, the 3rd and 4th ones are of interest: The BSP CPU
	 * sends out IPIs to APs in the order that the APs appear in the MP/ACPI
	 * tables. Even though the ACPI/MP tables require that the BSP appear
	 * as the first entry, there is no guarantee this will happen, so the
	 * BSP may send out IPIs before it itself is given a chance to execute
	 * this code which sets up its LAPIC.
	 *
	 * According to the 3rd provision then, this is still safe, even if the
	 * BSP's LAPIC was in soft-disable mode. The 4th point is of interest
	 * simply because it implies that we should explicitly ensure that the
	 * LAPIC is soft-enabled before trying to set up the rest of the LAPIC
	 * operating state.
	 **/
	if (X86Lapic::mapLapicMem() != ERROR_SUCCESS)
	{
		panic(FATAL CPUSTREAM"%d: setupLapic(): Failed to map LAPIC.\n",
			parent->cpuId);
	};

	// See above, explicitly enable LAPIC before fiddling with LVT regs.
	softEnable();
	spurious.setupSpuriousVector(parent);
	// Use vector 0xFF as the LAPIC error vector.
	error.setupLvtError(parent);
	ipi.setupIpis(parent);

	// This is called only when the CPU is ready to take IPIs.
	parent->interCpuMessager.bind();
	lint.setupLints(parent);

	return ERROR_SUCCESS;
}

