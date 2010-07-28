
#include <kernel/common/interruptTrib.h>

void interruptTrib_irqEntry(taskContextS *regs)
{
	interruptTrib.irqMain(regs);
}

error_t interruptTribC::initialize1(void)
{
	/**	EXPLANATION:
	 * Steps:
	 *	1. Install the hardware vector table.
	 *	2. Run the chipset's interruptInit function.
	 *	3. exit (I think).
	 **/
}

