#ifndef _INTERRUPT_TRIB_H
	#define _INTERRUPT_TRIB_H

	#include <arch/interrupts.h>
	#include <arch/taskContext.h>
	#include <chipset/zkcm/irqControl.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/ptrlessList.h>
	#include <__kclasses/hardwareIdList.h>
	#include <kernel/common/tributary.h>
	#include <kernel/common/interruptTrib/zkcmIsrFn.h>


#define INTTRIB					"InterruptTrib: "

// Applies to any ISR the chipset registered during intializeInterrupts().
#define INTTRIB_VECTOR_FLAGS_BOOTISR		(1<<1)
// States that the vector holds an exception.
#define INTTRIB_VECTOR_FLAGS_EXCEPTION		(1<<2)
// The exception on the vector must be called again just before executing IRET.
#define INTTRIB_VECTOR_FLAGS_EXCEPTION_POSTCALL	(1<<3)
// Declares that the vector holds a syscall handler and is an SWI vector.
#define INTTRIB_VECTOR_FLAGS_SWI		(1<<4)

// Zambezii Kernel Chipset Module.
#define INTTRIB_ISR_DRIVERTYPE_ZKCM		(0x0)
// Uniform Driver Interface.
#define INTTRIB_ISR_DRIVERTYPE_UDI		(0x1)

#define INTTRIB_IRQPIN_TRIGGMODE_LEVEL		0
#define INTTRIB_IRQPIN_TRIGGMODE_EDGE		1

class interruptTribC
:
public tributaryC
{
public:
	interruptTribC(void);
	error_t initialize(void);
	error_t initialize2(void);
	~interruptTribC(void) {};

public:
	// Will Eventually provide a code injection API for usermode drivers.
	status_t zkcmRegisterIsr(zkcmIsrFn *isr, uarch_t flags);
	void zkcmRemoveIsr(zkcmIsrFn *isr);

	void installException(
		uarch_t vector, __kexceptionFn *exception, uarch_t flags);

	void removeException(uarch_t vector);

	void irqMain(taskContextS *regs);

	// Called only by ZKCM IRQ Control module related code.
	void registerIrqPins(ubit16 nPins, zkcmIrqPinS *list);
	void removeIrqPins(ubit16 nPins, zkcmIrqPinS *list);

	void dumpIrqPins(void);
	void dumpExceptions(void);
	void dumpMsiIrqs(void);
	void dumpUnusedVectors(void);

private:
	// These two are architecture specific.
	void installHardwareVectorTable(void);
	void installExceptions(void);

public:
	struct isrDescriptorS
	{
		ptrlessListC<isrDescriptorS>::headerS	listHeader;
		ubit32		flags;
		// For profiling.
		uarch_t		nHandled;
		// NOTE: Should actually point to the bus driver instance.
		uarch_t		processId;
		zkcmIsrFn	*isr;
		ubit8		driverType;
		// Miscellaneous driver only use.
		ubit32		scratch;
	};

	struct vectorDescriptorS
	{
		uarch_t			flags;
		// For debugging.
		uarch_t			nUnhandled;

		/**	NOTE:
		 * Each vector may have one exception, and any number of other
		 * handlers. If an exception handler is installed on the
		 * vector, it is run first. Userspace and drivers are not
		 * allowed to install exceptions. They are only installed by
		 * the kernel at boot as required.
		 *
		 * The pointer to the exception handler on a vector is also the
		 * pointer to the vector's SWI handler if one exists on that
		 * vector. A vector which is to be used for syscall entry is not
		 * allowed to have other types of handlers on it as well. No
		 * ISRs or exceptions are permitted alongside an SWI handler on
		 * any vector.
		 **/
		__kexceptionFn		*exception;
		/* Each vector has a list of ISRs which have been registered to
		 * it. There is no limit to the number that may be installed on
		 * a vector, but in time the kernel will be augmented with the
		 * ability to balance the number of devices presenting on the
		 * same vector.
		 *
		 * If there is an SWI (syscall) handler on a vector, then no
		 * other type of handler is allowed to exist alongside it.
		 **/
		ptrlessListC<isrDescriptorS>	isrList;
	};

	struct irqPinDescriptorS
	{
		ubit8		triggerMode;
		ptrlessListC<isrDescriptorS>	irqList;
	};

	vectorDescriptorS	msiIrqTable[ARCH_IRQ_NVECTORS];
	hardwareIdListC		pinIrqTable;
	ubit16			pinIrqTableCounter;
};

extern interruptTribC		interruptTrib;
extern "C" void interruptTrib_irqEntry(taskContextS *regs);

#endif

