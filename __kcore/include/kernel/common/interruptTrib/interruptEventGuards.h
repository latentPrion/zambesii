#ifndef _INTERRUPT_EVENT_GUARDS_H
	#define _INTERRUPT_EVENT_GUARDS_H

	#include <config.h>
	#include <arch/cpuControl.h>
	#include <kernel/common/cpuTrib/cpuStream.h>

/**
 * RAII class to handle interrupt event counter enter/exit
 * Ensures event counters are properly incremented on enter and
 * decremented on exit regardless of how the function returns
 */
class IrqEventCounterGuard
{
public:
	IrqEventCounterGuard(CpuStream *stream)
	: cpuStream(stream), released(false)
	{
		cpuStream->asyncInterruptEvent.enter();
#ifdef CONFIG_DEBUG_INTERRUPTS
		cpuStream->irqEvent.enter();
#endif
	}

	~IrqEventCounterGuard()
		{ if (!released) { release(); } }

	void release()
	{
		released = true;
#ifdef CONFIG_DEBUG_INTERRUPTS
		cpuStream->irqEvent.exit();
#endif
		cpuStream->asyncInterruptEvent.exit();
	}

private:
	CpuStream	*cpuStream;
	sbit8		released;
};

/**
 * RAII class to handle exception event counter enter/exit
 * Ensures exception event counters are properly incremented on enter and
 * decremented on exit regardless of how the function returns
 */
class ExceptionEventCounterGuard
{
public:
	ExceptionEventCounterGuard(CpuStream *stream)
	: cpuStream(stream), released(false)
	{
#ifdef CONFIG_DEBUG_INTERRUPTS
		cpuStream->syncInterruptEvent.enter();
		cpuStream->excEvent.enter();
#endif
	}

	~ExceptionEventCounterGuard()
		{ if (!released) { release(); } }

	void release()
	{
		released = true;
#ifdef CONFIG_DEBUG_INTERRUPTS
		cpuStream->excEvent.exit();
		cpuStream->syncInterruptEvent.exit();
#endif
	}

private:
	CpuStream	*cpuStream;
	sbit8 		released;
};


/**
 * RAII class to handle local interrupt enable/disable
 * Enables interrupts in constructor and disables in destructor
 * Used for nested interrupt handling
 */
class LocalInterruptSignalGuard
{
public:
	LocalInterruptSignalGuard(bool enable = true)
	: shouldEnable(enable)
	{
		if (shouldEnable) { cpuControl::enableInterrupts(); }
	}

	~LocalInterruptSignalGuard()
		{ if (shouldEnable) { cpuControl::disableInterrupts(); } }

private:
	bool	shouldEnable;
};


#endif
