#ifndef _INTERRUPT_TRIB_H
	#define _INTERRUPT_TRIB_H

	#include <arch/interrupts.h>
	#include <arch/taskContext.h>
	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/tributary.h>

// Pass this to register an exclusive ISR.
#define INTERRUPTTRIB_FLAGS_EXCLUSIVE		(1<<0)

// Denotes a vector with only one ISR statically (non-shared).
#define INTERRUPTTRIB_VECTOR_FLAGS_SINGLE	(1<<0)

class interruptTribC
:
public tributaryC
{
public:
	typedef status_t (isrFn)();

public:
	interruptTribC(void) {};
	// Architecture specific.
	error_t initialize1(void);
	~interruptTribC(void) {};

public:
	// Will Eventually provide a code injection API for usermode drivers.
	status_t registerIsr(void (*vaddr)(), uarch_t flags);
	void removeIsr(void (*vaddr)());

	void irqMain(taskContextS *regs);

public:
	struct isrS
	{
		isrS			*next;
		uarch_t			flags;
		isrFn			*isr;
	};

	struct vectorDescriptorS
	{
		uarch_t		flags;

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
	vectorDescriptorS		isrTable;
};

extern interruptTribC		interruptTrib;
extern "C" void interruptTrib_irqEntry(taskContextS *regs);

#endif

