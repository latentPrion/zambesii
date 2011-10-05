#ifndef _INTERRUPT_TRIB_H
	#define _INTERRUPT_TRIB_H

	#include <arch/interrupts.h>
	#include <arch/taskContext.h>
	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/tributary.h>
	#include <kernel/common/interruptTrib/isrFn.h>

/**	EXPLANATION:
 * When Zambezii receives an IRQ, it quickly saves context (much like any other
 * kernel), then enters the Interrupt Tributary's main interrupt handling
 * routine (void interruptTribC::irqMain(taskContextS *regs)).
 *
 * On encountering an unknown interrupt, if there are no ISRs registered on the
 * nefarious vector, the kernel will increment the 'nUnhandled' counter on that
 * vector, and then attempt to ask the chipset support code for assistance. The
 * chipset is expected to mask that interrupt.
 *
 * The kernel will not ask the chipset support code to mask a vector if there
 * is at least one ISR registered there. In that case, the kernel will just have
 * to suck it up and increment the nUnhandled counter, since there is at least
 * one device that is known, and is raising a valid IRQ on that vector.
 *
 * If however, later on, for a vector with no handlers, the kernel receives a
 * request to register a handler for that vector, the kernel will ask the
 * chipset to unmask that interrupt.
 *
 * Technically, the kernel will ask the chipset to unmask each vector the first
 * time a handler is registered on that vector. And whenever a handler is
 * unregistered from a vector, such that there are no more ISRs on that vector,
 * the kernel will ask the chipset code to mask that vector once again.
 *
 * In this routine, the interrupt number is used as an index into an array of
 * vector descriptors. Each vector descriptor can be one of three things:
 *	1. An unused vector. (An interrupt coming in on an unused vector means
 *	   that the chipset is raising an unknown IRQ.) A vector is seen as
 *	   unused when there is no pointer to an ISR for that vector, whether
 *	   shared or unshared.
 *
 *	2. An unshared vector, which only holds one ISR. If an IRQ comes in on
 *	   an unshared vector, and the ISR
 *
 **/

// Denotes a vector with only one ISR statically (non-shared).
#define INTERRUPTTRIB_VECTOR_FLAGS_EXCLUSIVE	(1<<0)
// Applies to any ISR the chipset registered during intializeInterrupts().
#define INTERRUPTTRIB_VECTOR_FLAGS_BOOTISR	(1<<1)
// States that the vector holds an exception.
#define INTERRUPTTRIB_VECTOR_FLAGS_EXCEPTION	(1<<4)

#define INTERRUPTTRIB_ISR_FLAGS_LEVEL_TRIGGERED	(1<<0)

class interruptTribC
:
public tributaryC
{
public:
	interruptTribC(void);
	error_t initialize(void);
	~interruptTribC(void) {};

public:
	// Will Eventually provide a code injection API for usermode drivers.
	status_t registerIsr(isrFn *isr, uarch_t flags);
	void removeIsr(isrFn *isr);

	void installException(uarch_t vector, exceptionFn *exception);
	void removeException(uarch_t vector);

	void irqMain(taskContextS *regs);

private:
	// These two are architecture specific.
	void installHardwareVectorTable(void);
	void installExceptions(void);

public:
	struct isrS
	{
		isrS		*next;
		ubit32		flags;
		// For profiling.
		uarch_t		nHandled;
		// NOTE: Should actually point to the bus driver instance.
		uarch_t		processId;
		isrFn		*isr;
	};

	struct vectorDescriptorS
	{
		uarch_t		flags;
		// For debugging.
		uarch_t		nUnhandled;

		/**	NOTE:
		 * Each vector may have one exception, and any number of other
		 * handlers. If an exception handler is installed on the
		 * vector, it is run first. Userspace and drivers are not
		 * allowed to install exceptions. They are only installed by
		 * the kernel at boot as required.
		 **/
		exceptionFn	*exception;
		isrS		*isrList;
	};
	vectorDescriptorS		isrTable[ARCH_IRQ_NVECTORS];
};

extern interruptTribC		interruptTrib;
extern "C" void interruptTrib_irqEntry(taskContextS *regs);

#endif

