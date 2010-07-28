#ifndef _INTERRUPT_TRIB_H
	#define _INTERRUPT_TRIB_H

	#include <arch/interrupts.h>
	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/tributary.h>

class interruptTribC
:
public tributaryC
{
public:
	tributaryC(void) {};
	error_t initialize(void);
	~tributaryC(void) {};

public:
	void		(*vectorHandlers[ARCH_IRQ_NVECTORS])()
};

extern interruptTribC		interruptTrib;

#endif

