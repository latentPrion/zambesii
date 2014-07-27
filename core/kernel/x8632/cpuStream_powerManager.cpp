
#include <__ksymbols.h>
#include <commonlibs/libx86mp/libx86mp.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/cpuTrib/cpuStream.h>
#include <kernel/common/processTrib/processTrib.h>


/**	EXPLANATION:
 * According to the MP specification, only newer LAPICs require the
 * INIT-SIPI-SIPI sequence; older LAPICs will be sufficient with a
 * single INIT.
 *
 * The kernel therefore postulates the following when waking CPUs:
 *	1. The only way to know what type of LAPIC exists on a CPU
 *	   without having already awakened that CPU is for the chipset
 *	   to tell the kernel (through MP tables).
 *	2. If there are no MP tables, then the chipset is new enough
 *	   to not be using older CPUs.
 *
 * This means that if the kernel checks for MP tables and finds none,
 * then it will assume a newer CPU is being awakened and send the
 * INIT-SIPI-SIPI sequence. Otherwise, it will check the MP tables and
 * see if the CPU in question has an integrated or external LAPIC and
 * decide whether or not to use SIPI-SIPI after the INIT.
 **/

static sarch_t cpuHasOlderNonIntegratedLapic(cpu_t cpuId)
{
	x86Mp::sCpuConfig	*cpu;
	void		*handle=0;
	uarch_t		pos=0;
	sarch_t		isNewerCpu=1;

	// Scan MP tables.
	x86Mp::initializeCache();
	if (!x86Mp::mpConfigTableIsMapped())
	{
		if (x86Mp::findMpFp())
		{
			if (!x86Mp::mapMpConfigTable())
			{
				printf(WARNING CPUPWRMGR"%d: bootPowerOn: "
					"Unable to map MP config table.\n",
					cpuId);

				return 0;
			};
		}
		else
		{
			printf(NOTICE CPUPWRMGR"%d: bootPowerOn: No MP "
				"tables; Assuming newer CPU and INIT-SIPI-SIPI."
				"\n", cpuId);

			return 0;
		};
	};

	/* If CPU is not found in SMP tables, assume it is being hotplugged and
	 * the system admin who is inserting it is knowledgeable and won't
	 * hotplug an older CPU. I don't even think older CPUs support hotplug,
	 * but I'm sure specific chipsets may be custom built for all kinds of
	 * things.
	 **/
	cpu = x86Mp::getNextCpuEntry(&pos, &handle);
	for (; cpu != NULL; cpu = x86Mp::getNextCpuEntry(&pos, &handle))
	{
		if (cpu->lapicId != cpuId) { continue; };

		// Check for on-chip APIC.
		if (!FLAG_TEST(cpu->featureFlags, (1<<9))
			&& !(cpu->lapicVersion & 0xF0))
		{
			isNewerCpu = 0;
			printf(WARNING CPUPWRMGR"%d: bootPowerOn: Old "
				"non-integrated LAPIC.\n",
				cpuId);
		}
	};

	return !isNewerCpu;
}

#include <arch/cpuControl.h>
status_t cpuStream::PowerManager::bootPowerOn(ubit32)
{
	error_t		ret;

	if (!x86LapicC::lapicMemIsMapped()) { return ERROR_UNINITIALIZED; };

	if (!cpuTrib.usingChipsetSmpMode())
	{
		printf(ERROR CPUPWRMGR"%d: bootPowerOn: Attempt to "
			"power on a CPU with usingChipsetSmpMode=0!\n",
			parent->cpuId);

		return ERROR_CRITICAL;
	};

	// Init IPI: Always with vector = 0.
	ret = parent->lapic.ipi.sendPhysicalIpi(
		x86LAPIC_IPI_TYPE_INIT,
		0,
		x86LAPIC_IPI_SHORTDEST_NONE,
		parent->cpuId);

	if (ret != ERROR_SUCCESS)
	{
		printf(ERROR CPUPWRMGR"%d: bootPowerOn: INIT IPI timed out."
			"\n", parent->cpuId);
	};

	/**	FIXME:
	 * Even if the IPI above timed out, we still set a
	 * timer for the bootup here, simply because doing otherwise will
	 * complicate the sequence -- returning without setting a timer
	 * will cause the calling CPU Trib loop to get stuck waiting forever.
	 **/
	// Set a 10 millisecond timeout.
	processTrib.__kgetStream()->timerStream.createRelativeOneshotEvent(
		sTimestamp(0, 0, 10000000), 0, 0, parent);

	return ERROR_SUCCESS;
}

void cpuStream::PowerManager::bootWaitForCpuToPowerOn(void)
{
	MessageStream::sIterator	event;
	cpuStream			*cs;
	sarch_t				loopAgain;
	uarch_t				sipiVector;
	error_t				err;

	sipiVector = (((uarch_t)&__kcpuPowerOnTextStart) >> 12) & 0xFF;

	do
	{
		loopAgain = 0;

		processTrib.__kgetStream()->timerStream.pullEvent(
			0, (TimerStream::sTimerMsg *)&event);

		cs = reinterpret_cast<cpuStream *>( event.header.privateData );

		switch (cs->powerManager.getPowerStatus())
		{
		case OFF:
			// If non-integrated LAPIC, boot failed. Else send SIPI.
			if (cpuHasOlderNonIntegratedLapic(cs->cpuId))
			{
				cs->powerManager.setPowerStatus(FAILED_BOOT);
				printf(WARNING CPUPWRMGR"%d: CPU failed to "
					"boot.\n",
					cs->cpuId);

				break;
			}
			else
			{
				// Intgr. LAPIC. Send first SIPI, set timeout.
				cs->powerManager.setPowerStatus(POWERING_ON);
				err = parent->lapic.ipi.sendPhysicalIpi(
					x86LAPIC_IPI_TYPE_SIPI,
					sipiVector,
					x86LAPIC_IPI_SHORTDEST_NONE,
					cs->cpuId);

				if (err != ERROR_SUCCESS)
				{
					printf(ERROR CPUPWRMGR"%d: "
						"bootPowerOn: SIPI1 timed out."
						"\n", cs->cpuId);

					cs->powerManager.setPowerStatus(
						FAILED_BOOT);

					break;
				};

				processTrib.__kgetStream()->timerStream
					.createRelativeOneshotEvent(
						sTimestamp(0, 0, 200000),
						0, 0, cs);

				loopAgain = 1;
				break;
			};

		case POWERING_ON:
			// Integrated LAPIC. Send second SIPI and set timeout.
			cs->powerManager.setPowerStatus(POWERING_ON_RETRY);
			err = parent->lapic.ipi.sendPhysicalIpi(
				x86LAPIC_IPI_TYPE_SIPI,
				sipiVector,
				x86LAPIC_IPI_SHORTDEST_NONE,
				cs->cpuId);

			if (err != ERROR_SUCCESS)
			{
				printf(ERROR CPUPWRMGR"%d: bootPowerOn: "
					"SIPI2 timed out.\n",
					cs->cpuId);

				cs->powerManager.setPowerStatus(FAILED_BOOT);
				break;
			};

			processTrib.__kgetStream()->timerStream
				.createRelativeOneshotEvent(
					sTimestamp(0, 0, 200000),
					0, 0, cs);

			loopAgain = 1;
			break;

		case POWERING_ON_RETRY:
			// Integrated LAPIC that failed to boot.
			cs->powerManager.setPowerStatus(FAILED_BOOT);
			printf(WARNING CPUPWRMGR"%d: CPU failed to boot.\n",
				cs->cpuId);

			break;

		default:
//printf(NOTICE"After INIT ipi on CPU %d, local ints are %d, nLocks %d.\n", cs->cpuId, cpuControl::interruptsEnabled(), cpuTrib.getCurrentCpuStream()->taskStream.getCurrentTask()->nLocksHeld);
			// CPU successfully booted.
			printf(NOTICE CPUPWRMGR"%d: Successfully booted.\n",
				cs->cpuId);

			break;
		};
	} while (loopAgain);
}

