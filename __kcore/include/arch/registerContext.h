#include <arch/arch_include.h>
#include ARCH_INCLUDE(registerContext.h)

extern "C"
{
	/** EXPLANATION:
	 * If asyncInterruptNestingLevel is 0 (i.e: local CPU is not handling
	 * an async interrupt event), this function will save the current thread's
	 * register context and call TaskStream::pull(). Else it will do nothing
	 * and return immediately because we should not be doing context switches
	 * in the middle of an async interrupt event.
	 *
	 * Switching tasks in the middle of an async interrupt event is not allowed
	 * because it would save the interrupt's register context onto the stack of
	 * the current thread. This would effectively bind the fate of that interrupt
	 * event to the current thread. For async interrupts, this means that the
	 * interrupt would become, in effect, a synchronous event. Async events are
	 * supposed to be asynchronous. They aren't supposed to be bound to any
	 * particular thread.
	 *
	 * For sync interrupts, we permit immediate context switches because they
	 * should already be supported by default: syscalls are in fact synchronous
	 * events. They are also the exact place where most sched ops will be
	 * invoked. So we already have to support immediate context switches
	 * in sync events.
	 **/
	void saveContextAndCallPull(
		void *cpuSleepStackBaseAddr, uarch_t asyncInterruptNestingLevel);

	/** EXPLANATION:
	 * "tc" is a pointer to a RegisterContext.
	 * The function will load the context and jump to the saved program counter.
	 **/
	void loadContextAndJump(RegisterContext *tc);
}

