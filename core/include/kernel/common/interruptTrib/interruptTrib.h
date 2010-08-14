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
// Denotes a vector which has been masked off at its PIC.
#define INTERRUPTTRIB_VECTOR_FLAGS_MASKED	(1<<2)
// Identifies a vector which is unknown, and cannot be masked (trouble).
#define INTERRUPTTRIB_VECTOR_FLAGS_MASKFAILS	(1<<3)
// States that the vector holds a single exception handler, and no ISRs.
#define INTERRUPTTRIB_VECTOR_FLAGS_EXCEPTION	(1<<4)

// States that this shared ISR is an exception handler and not a hardware ISR.
#define INTERRUPTTRIB_ISR_FLAGS_EXCEPTION	(1<<0)

class interruptTribC
:
public tributaryC
{
public:
	interruptTribC(void);
	// Architecture specific.
	error_t initialize1(void);
	~interruptTribC(void) {};

public:
	// Will Eventually provide a code injection API for usermode drivers.
	status_t registerIsr(isrFn *isr, uarch_t flags);
	void removeIsr(isrFn *isr);

	void irqMain(taskContextS *regs);

public:
	struct isrS
	{
		isrS		*next;
		uarch_t		flags;
		uarch_t		processId;
		isrFn		*isr;
	};

	struct vectorDescriptorS
	{
		uarch_t		flags;
		uarch_t		processId;
		uarch_t		nUnhandled;

		/* Depending on the flags, this may be a direct pointer to an
		 * ISR in the kernel's address space (in the case that this
		 * ISR was successfully registered as a non-shared ISR), or
		 * it may be a pointer to a list of ISRs for this vector.
		 **/
		union {
			isrFn	*isr;
			isrS	*list;
		} handler;
	};
	vectorDescriptorS		isrTable[ARCH_IRQ_NVECTORS];
};

extern interruptTribC		interruptTrib;
extern "C" void interruptTrib_irqEntry(taskContextS *regs);

#endif

