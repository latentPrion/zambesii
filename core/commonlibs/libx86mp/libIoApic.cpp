
#include <debug.h>
#include <arch/arch.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kstdlib/__kcxxlib/new>
#include <commonlibs/libx86mp/libx86mp.h>
#include <commonlibs/libacpi/libacpi.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/memoryTrib/memoryTrib.h>


static x86IoApic::cacheS	cache;

void x86IoApic::initializeCache(void)
{
	if (cache.magic != x86IOAPIC_MAGIC)
	{
		flushCache();
		cache.magic = x86IOAPIC_MAGIC;
	};
}

void x86IoApic::flushCache(void)
{
	cache.magic = 0;
	cache.mapped = 0;
	cache.nIoApics = 0;
}

void x86IoApic::dump(void)
{
	__kprintf(NOTICE x86IOAPIC"Dumping. Cache @v 0x%p magic: 0x%x.\n"
		"\tmapped %d, nIoApics %d.\n",
		&cache, cache.magic, cache.mapped, cache.nIoApics);
}

sarch_t x86IoApic::ioApicsAreDetected(void)
{
	if (cache.magic != x86IOAPIC_MAGIC) { return 0; };

	return cache.mapped;
}

ubit16 x86IoApic::getNIoApics(void)
{
	if (cache.magic != x86IOAPIC_MAGIC) { return 0; };

	return cache.nIoApics;
}

x86IoApic::ioApicC *x86IoApic::getIoApic(ubit8 id)
{
	return reinterpret_cast<ioApicC *>( cache.ioApics.getItem(id) );
}

static error_t rsdtDetectIoApics(void)
{
	error_t			ret;
	x86IoApic::ioApicC	*tmp;
	acpi_rsdtS		*rsdt;
	acpi_rMadtS		*madt;
	acpi_rMadtIoApicS	*ioApicEntry;
	void			*handle, *handle2, *context;
	ubit8			nIoApics;

	/* The fact that we're here means there is an RSDT, so look for an MADT
	 * etc and get info on IO-APICs.
	 **/
	rsdt = acpi::getRsdt();
	handle = context = __KNULL;
	madt = acpiRsdt::getNextMadt(rsdt, &context, &handle);
	for (; madt != __KNULL;
		madt = acpiRsdt::getNextMadt(rsdt, &context, &handle))
	{
		handle2 = __KNULL;
		ioApicEntry = acpiRMadt::getNextIoApicEntry(madt, &handle2);
		for (; ioApicEntry != __KNULL;
			ioApicEntry =
				acpiRMadt::getNextIoApicEntry(madt, &handle2))
		{
			tmp = new x86IoApic::ioApicC(
				ioApicEntry->ioApicId,
				ioApicEntry->ioApicPaddr);

			if (tmp == __KNULL) { return ERROR_MEMORY_NOMEM; };

			ret = tmp->initialize();
			if (ret != ERROR_SUCCESS)
			{
				__kprintf(ERROR x86IOAPIC"Failed to init APIC "
					"%d.\n",
					ioApicEntry->ioApicId);

				delete tmp;
				return ret;
			};

			ret = cache.ioApics.addItem(ioApicEntry->ioApicId, tmp);
			if (ret != ERROR_SUCCESS)
			{
				__kprintf(ERROR x86IOAPIC"Failed to add APIC "
					"%d to the list.\n",
					ioApicEntry->ioApicId);

				delete tmp;
				return ret;
			};

			nIoApics++;
			ioApicEntry = acpiRMadt::getNextIoApicEntry(
				madt, &handle2);
		};
		acpiRsdt::destroySdt(reinterpret_cast<acpi_sdtS *>( madt ));
	};

	cache.mapped = 1;
	cache.nIoApics = nIoApics;
	return ERROR_SUCCESS;
}

error_t x86IoApic::detectIoApics(void)
{
	error_t			ret;
	x86_mpCfgIoApicS	*ioApicEntry;
	uarch_t			pos;
	void			*handle;
	ubit8			nIoApics=0;
	ioApicC			*tmp;

	if (cache.magic != x86IOAPIC_MAGIC) { return ERROR_INVALID_ARG; };
	if (ioApicsAreDetected()) { return ERROR_SUCCESS; };

	acpi::initializeCache();
	if (!acpi::rsdpFound())
	{
		if (acpi::findRsdp() != ERROR_SUCCESS)
		{
			__kprintf(ERROR x86IOAPIC"Unable to find RSDP; falling "
				"back to MP tables.\n");

			goto tryMpTables;
		};
	};

#if defined(__64_BIT__) || defined(CONFIG_ARCH_x86_32_PAE)
		// XSDT code here.
#else
	if (acpi::testForRsdt())
	{
		if (acpi::mapRsdt() != ERROR_SUCCESS)
		{
			__kprintf(ERROR x86IOAPIC"Unable to map RSDT. "
				"Falling back to MP tables.\n");

			goto tryMpTables;
		};

		// Use RSDT to find MADT and parse for IO APICs.
		return rsdtDetectIoApics();
	};
#endif

tryMpTables:
	x86Mp::initializeCache();
	if (!x86Mp::mpConfigTableIsMapped())
	{
		if (x86Mp::findMpFp() == __KNULL)
		{
			__kprintf(WARNING x86IOAPIC"Failed to find MP FP.\n");
			return ERROR_GENERAL;
		};

		if (x86Mp::mapMpConfigTable() == __KNULL)
		{
			__kprintf(WARNING x86IOAPIC"Failed to map MP config "
				"table.\n");

			return ERROR_MEMORY_VIRTUAL_PAGEMAP;
		};
	};

	pos = 0; handle = __KNULL;
	ioApicEntry = x86Mp::getNextIoApicEntry(&pos, &handle);
	for (; ioApicEntry != __KNULL;
		ioApicEntry = x86Mp::getNextIoApicEntry(&pos, &handle))
	{
		if (!__KFLAG_TEST(
			ioApicEntry->flags, x86_MPCFG_IOAPIC_FLAGS_ENABLED))
		{
			__kprintf(NOTICE x86IOAPIC"Skipping unsafe IO APIC %d."
				"\n", ioApicEntry->ioApicId);

			continue;
		};

		tmp = new ioApicC(
			ioApicEntry->ioApicId, ioApicEntry->ioApicPaddr);

		// Masks all IRQs and sets safe known state.
		ret = tmp->initialize();
		if (ret != ERROR_SUCCESS)
		{
			__kprintf(ERROR x86IOAPIC"Failed to init IOAPIC %d.\n",
				ioApicEntry->ioApicId);

			delete tmp;
			return ret;
		};

		ret = cache.ioApics.addItem(ioApicEntry->ioApicId, tmp);
		if (ret != ERROR_SUCCESS)
		{
			__kprintf(ERROR x86IOAPIC"Failed to add APIC %d to "
				"the list.\n",
				ioApicEntry->ioApicId);

			delete tmp;
			return ret;
		};

		nIoApics++;
	};

	cache.mapped = 1;
	cache.nIoApics = nIoApics;
	return ERROR_SUCCESS;
}

x86IoApic::ioApicC *x86IoApic::getIoApicFor(ubit16 __kpin)
{
	sarch_t		context;
	ioApicC		*ret;

	context = cache.ioApics.prepareForLoop();
	ret = reinterpret_cast<ioApicC*>( cache.ioApics.getLoopItem(&context) );

	for (; ret != __KNULL;
		ret = (ioApicC *)cache.ioApics.getLoopItem(&context))
	{
		if (__kpin > ret->get__kPinBase()
			&& __kpin < (ret->get__kPinBase() + ret->getNIrqs() - 1)
			)
		{
			return ret;
		};
	};

	return __KNULL;
}

void x86IoApic::maskAll(void)
{
	ioApicC		*ioApic;
	sarch_t		context;

	context = cache.ioApics.prepareForLoop();
	ioApic = (ioApicC *)cache.ioApics.getLoopItem(&context);

	for (; ioApic != __KNULL;
		ioApic = (ioApicC *)cache.ioApics.getLoopItem(&context))
	{
		ioApic->maskAll();
	};
}

void x86IoApic::unmaskAll(void)
{
	ioApicC		*ioApic;
	sarch_t		context;

	context = cache.ioApics.prepareForLoop();
	ioApic = (ioApicC *)cache.ioApics.getLoopItem(&context);

	for (; ioApic != __KNULL;
		ioApic = (ioApicC *)cache.ioApics.getLoopItem(&context))
	{
		ioApic->unmaskAll();
	};
}

error_t x86IoApic::identifyIrq(uarch_t physicalId, ubit16 *__kpin)
{
	sarch_t		context;
	ioApicC		*ioApic;

	/* Truth be told, the only way to know for this case is by using
	 * the ACPI IDs.
	 *
	 *	EXPLANATION:
	 * We ask each IO-APIC in turn if it can identify the IRQ based on its
	 * knowledge of the ACPI IDs of its pins. If none of them can, we just
	 * assume the pin doesn't exist.
	 **/
	context = cache.ioApics.prepareForLoop();
	ioApic = (ioApicC *)cache.ioApics.getLoopItem(&context);

	for (; ioApic != __KNULL;
		ioApic = (ioApicC *)cache.ioApics.getLoopItem(&context))
	{
		if (ioApic->identifyIrq(physicalId, __kpin) == ERROR_SUCCESS) {
			return ERROR_SUCCESS;
		};
	};

	return ERROR_INVALID_ARG_VAL;
}

