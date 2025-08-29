
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
	void			*handle=0;
	uarch_t			pos=0;
	sarch_t			isNewerCpu=1;

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

typedef void (CpuStreamPowerOnCbFn)(
	MessageStream::sHeader *msg,
	CpuStream::PowerManager *powerManager,
	CpuStream::PowerManager::sCpuPowerOnMsg *response);

class CpuStreamPowerOnCb
: public MessageStreamCallback<CpuStreamPowerOnCbFn *>
{
public:
	CpuStreamPowerOnCb(
		CpuStreamPowerOnCbFn *function,
		CpuStream::PowerManager *_powerManager,
		CpuStream::PowerManager::sCpuPowerOnMsg *_response)
	: MessageStreamCallback<CpuStreamPowerOnCbFn *>(function),
	powerManager(_powerManager), response(_response)
	{}

	virtual void operator()(MessageStream::sHeader *msg)
		{ function(msg, powerManager, response); }

public:
	CpuStream::PowerManager			*powerManager;
	CpuStream::PowerManager::sCpuPowerOnMsg	*response;
};

static inline uarch_t calculateSipiVector(void)
{
	return (((uarch_t)&__kcpuPowerOnTextStart) >> 12) & 0xFF;
}

#include <arch/cpuControl.h>
#include <kernel/common/timerTrib/timerTrib.h>
#include "../../chipset/ibmPc/i8254.h"

static CpuStreamPowerOnCbFn		bootPowerOn_contd1;

status_t CpuStream::PowerManager::bootPowerOnReq(ubit32, void *privateData)
{
	status_t						ret;
	HeapObj<CpuStream::PowerManager::sCpuPowerOnMsg>	response;

	response = new CpuStream::PowerManager::sCpuPowerOnMsg(
		cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread()
			->getFullId(),
		CpuStream::PowerManager::OP_BOOT_POWER_ON_REQ,
		0, privateData, this->parent);

	if (response == NULL) { return ERROR_MEMORY_NOMEM; };

	if (!X86Lapic::lapicMemIsMapped()) { return ERROR_UNINITIALIZED; };

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
		
		return ret;
	};

printf(FATAL CPUPWRMGR"bootPowerOn: about to create oneshot event.\n");
	/**	FIXME:
	 * Even if the IPI above timed out, we still set a
	 * timer for the bootup here, simply because doing otherwise will
	 * complicate the sequence -- returning without setting a timer
	 * will cause the calling CPU Trib loop to get stuck waiting forever.
	 **/
	// Set a 10 millisecond timeout.
	ret = processTrib.__kgetStream()->timerStream.createRelativeOneshotEvent(
		sTimestamp(0, 0, 10000000), 0,
		new CpuStreamPowerOnCb(
			&bootPowerOn_contd1, this, response.release()));
//printf(FATAL CPUPWRMGR"bootPowerOn: about to return.\n");
//assert_fatal(i8254Pit.irqEventMessagesEnabled());
//timerTrib.dump();
	return ERROR_SUCCESS;
}

static void bootPowerOn_contd1(
	MessageStream::sHeader *, CpuStream::PowerManager *cspm,
	CpuStream::PowerManager::sCpuPowerOnMsg *response
	)
{
	error_t					err;
	AsyncResponse				myResponse;
	const uarch_t				sipiVector =
		calculateSipiVector();

	myResponse(response);

	switch (cspm->getPowerStatus())
	{
	case CpuStream::PowerManager::OFF:
		// If non-integrated LAPIC, boot failed. Else send SIPI.
		if (cpuHasOlderNonIntegratedLapic(cspm->parent->cpuId))
		{
			cspm->setPowerStatus(
				CpuStream::PowerManager::FAILED_BOOT);

			printf(WARNING CPUPWRMGR"%d: CPU failed to "
				"boot.\n",
				cspm->parent->cpuId);

			myResponse(ERROR_INITIALIZATION_FAILURE);
			return;
		}
		else
		{
			// Intgr. LAPIC. Send first SIPI, set timeout.
			cspm->setPowerStatus(
				CpuStream::PowerManager::POWERING_ON);

			err = cspm->parent->lapic.ipi.sendPhysicalIpi(
				x86LAPIC_IPI_TYPE_SIPI,
				sipiVector,
				x86LAPIC_IPI_SHORTDEST_NONE,
				cspm->parent->cpuId);

			if (err != ERROR_SUCCESS)
			{
				printf(ERROR CPUPWRMGR"%d: "
					"bootPowerOn: SIPI1 timed out."
					"\n", cspm->parent->cpuId);

				cspm->setPowerStatus(
					CpuStream::PowerManager::FAILED_BOOT);

				myResponse(err); return;
			};

			err = processTrib.__kgetStream()->timerStream
				.createRelativeOneshotEvent(
					sTimestamp(0, 0, 200000),
					0, new CpuStreamPowerOnCb(
						&bootPowerOn_contd1,
						cspm, response));

			if (err != ERROR_SUCCESS)
			{
				printf(ERROR CPUPWRMGR"%d: "
					"bootPowerOn_contd2: "
					"Failed to create timer.\n",
					cspm->parent->cpuId);

				cspm->setPowerStatus(
					CpuStream::PowerManager::FAILED_BOOT);

				myResponse(err); return;
			};
		}

		myResponse(DONT_SEND_RESPONSE);
		return;

	case CpuStream::PowerManager::POWERING_ON:
		// Integrated LAPIC. Send second SIPI and set timeout.
		cspm->setPowerStatus(
			CpuStream::PowerManager::POWERING_ON_RETRY);
		err = cspm->parent->lapic.ipi.sendPhysicalIpi(
			x86LAPIC_IPI_TYPE_SIPI,
			sipiVector,
			x86LAPIC_IPI_SHORTDEST_NONE,
			cspm->parent->cpuId);
		if (err != ERROR_SUCCESS)
		{
			printf(ERROR CPUPWRMGR"%d: bootPowerOn: "
				"SIPI2 timed out.\n",
				cspm->parent->cpuId);

			cspm->setPowerStatus(
				CpuStream::PowerManager::FAILED_BOOT);

			myResponse(err); return;
		};

		err = processTrib.__kgetStream()->timerStream
			.createRelativeOneshotEvent(
				sTimestamp(0, 0, 200000), 0,
				new CpuStreamPowerOnCb(
					&bootPowerOn_contd1,
					cspm, response));

		if (err != ERROR_SUCCESS)
		{
			printf(ERROR CPUPWRMGR"%d: "
				"bootPowerOn_contd2: "
				"Failed to create timer.\n",
				cspm->parent->cpuId);

			cspm->setPowerStatus(
				CpuStream::PowerManager::FAILED_BOOT);

			myResponse(err); return;
		};

		myResponse(DONT_SEND_RESPONSE);
		return;

	case CpuStream::PowerManager::POWERING_ON_RETRY:
		// Integrated LAPIC that failed to boot.
		cspm->setPowerStatus(
			CpuStream::PowerManager::FAILED_BOOT);

		printf(WARNING CPUPWRMGR"%d: CPU failed to boot.\n",
			cspm->parent->cpuId);

		myResponse(ERROR_INITIALIZATION_FAILURE);
		return;

	default:
		// CPU successfully booted.
		printf(NOTICE CPUPWRMGR"%d: Successfully booted.\n",
			cspm->parent->cpuId);

		myResponse(ERROR_SUCCESS);
		return;

	}

	myResponse(ERROR_UNKNOWN);
}

