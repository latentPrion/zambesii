#ifndef _INTERRUPT_TRIB_H
	#define _INTERRUPT_TRIB_H

	#include <arch/interrupts.h>
	#include <arch/taskContext.h>
	#include <chipset/zkcm/irqControl.h>
	#include <chipset/zkcm/zkcmIsr.h>
	#include <chipset/zkcm/device.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/ptrList.h>
	#include <__kclasses/hardwareIdList.h>
	#include <kernel/common/tributary.h>
	#include <kernel/common/interruptTrib/__kexceptionFn.h>


#define INTTRIB					"InterruptTrib: "

// The exception on the vector must be called again just before executing IRET.
#define INTTRIB_EXCEPTION_FLAGS_POSTCALL	(1<<3)

class interruptTribC
:
public tributaryC
{
public:
	interruptTribC(void);
	// Sets up arch-specific exception handling and masks off ALL IRQ pins.
	error_t initializeExceptions(void);
	// Initializes the ZKCM IRQ Control mod, bus-pin mappings and IRQs.
	error_t initializeIrqManagement(void);
	~interruptTribC(void) {};

public:
	void installException(
		uarch_t vector, __kexceptionFn *exception, uarch_t flags);

	void removeException(uarch_t vector);

	// Containing namespace for the ZKCM sub-API.
	struct zkcmS
	{
		/**	NOTES:
		 * These functions deal with the registration and retire of
		 * ZKCM driver API ISRs. When UDI is ported, we will have
		 * a separate API for registering UDI ISRs.
		 *
		 * The kernel keeps pin-based IRQs and message-signaled IRQs
		 * separate. Pin-based IRQs are registered with registerPinIsr()
		 * and MSI IRQs are registered with registerMsiIsr().
		 *
		 * registerPinIsr():
		 *	Adds a pin based IRQ handler to the list of ISRs on the
		 *	specified __kpin. Does not unmask the __kpin if it
		 *	is not already unmasked -- the caller must explicitly
		 *	call __kpinEnable() to unmask the IRQ pin.
		 *
		 * retirePinIsr():
		 *	Removes a pin based IRQ handler from the list of ISRs
		 *	on the specified __kpin. This function /will/ mask the
		 *	IRQ off automatically if the ISR being removed is the
		 *	last ISR in the list for the specified __kpin.
		 *
		 * registerMsiIsr():
		 *	Adds an IRQ handler to the list of ISRs on the specified
		 *	interrupt vector of the CPU. An MSI ISR must not be
		 *	installed on a vector which has an exception installed
		 *	on it, or on a vector which was reserved by the CPU
		 *	architecture's manual. Such an attempt will be rejected.
		 *	This function does not "enable" the allocated MSI IRQ
		 *	automatically, and the caller must also call
		 *	enableMsiVector() as well, after calling this function.
		 *
		 * retireMsiIsr():
		 *	Removes an IRQ handler from the list of ISRs on the
		 *	specified interrupt vector of the CPU. This function
		 *	/will/ automatically disable/deallocate the MSI IRQ
		 *	upon completion.
		 **/
		error_t registerPinIsr(
			ubit16 __kpin, zkcmDeviceBaseC *dev, zkcmIsrFn *isr,
			uarch_t flags);

		sarch_t retirePinIsr(ubit16 __kpin, zkcmIsrFn *isr);

		error_t registerMsiIsr(
			uarch_t vector, zkcmDeviceBaseC *dev, zkcmIsrFn *isr,
			uarch_t flags);

		sarch_t retireMsiIsr(uarch_t vector, zkcmIsrFn *isr);
	} zkcm;

	error_t __kpinEnable(ubit16 __kpin);
	sarch_t __kpinDisable(ubit16 __kpin);
	error_t enableMsiVector(uarch_t vector);
	sarch_t disableMsiVector(uarch_t vector);

	// Called only by ZKCM IRQ Control module related code.
	void registerIrqPins(ubit16 nPins, zkcmIrqPinS *list);
	void removeIrqPins(ubit16 nPins, zkcmIrqPinS *list);

	// For debugging.
	void dumpIrqPins(void);
	void dumpExceptions(void);
	void dumpMsiIrqs(void);
	void dumpUnusedVectors(void);

	void pinIrqMain(registerContextC *regs);
	void msiIrqMain(registerContextC *regs);
	void swiMain(registerContextC *regs);
	void exceptionMain(registerContextC *regs);

private:
	// These two are architecture specific.
	void installHardwareVectorTable(void);
	void installExceptions(void);

private:
	struct isrDescriptorS
	{
		enum driverTypeE { ZKCM=0, UDI };

		isrDescriptorS(driverTypeE driverType)
		:
		driverType(driverType), flags(0), nHandled(0)
		{
			api.udi.isr = __KNULL;
			api.zkcm.isr = __KNULL;
		}

		driverTypeE	driverType;

		ubit32		flags;
		// Total number of IRQs claimed and handled so far.
		uarch_t		nHandled;
		union
		{
			struct
			{
				zkcmIsrFn	*isr;
				zkcmDeviceBaseC	*device;
			} zkcm;

			struct
			{
				void		*isr;
				processId_t	istId;
				void		*queue;
				void		*transList;
			} udi;
		} api;
	};

	friend void dumpIsrDescriptor(ubit16, isrDescriptorS *);

	struct vectorDescriptorS
	{
		vectorDescriptorS(void)
		:
		flags(0), nUnhandled(0),
		exception(__KNULL)
		{}

		enum vectorTypeE
		{
			UNCLAIMED=0, EXCEPTION, MSI_IRQ, SWI
		} type;

		// For debugging.
		uarch_t			flags;
		uarch_t			nUnhandled;

		/**	NOTE:
		 * Each vector may have either one exception, a list of IRQ
		 * handlers (ISRs) or an SWI handler. These are all mutually
		 * exclusive.
		 * 
		 * Exceptions may only be installed on a vector by the kernel
		 * itself. Userspace and drivers may not install exceptions on
		 * an interrupt vector.
		 *
		 * IRQ handlers may be installed by the kernel and by drivers,
		 * but not by userspace.
		 *
		 * SWI handlers may be installed by the kernel and by UXE
		 * modules, but not by drivers.
		 *
		 * The pointer to the exception handler on each vector
		 * (__kexceptionFn *exception) is also the pointer to the
		 * vector's SWI handler if one exists on that vector.
		 **/
		__kexceptionFn		*exception;

		/* Each interrupt vector has a list for possible MSI ISRs which
		 * may be installed on it if it is an MSI vector, and not of any
		 * other type (such as SWI or exception). There is no limit to
		 * the number of ISRs that may be installed on a vector, but in
		 * time the kernel will be augmented with the ability to balance
		 * the number of devices mapped to signal on the same vector.
		 **/
		ptrListC<isrDescriptorS>	isrList;
	};

	struct irqPinDescriptorS
	{
		irqPinDescriptorS(void)
		:
		flags(0), nUnhandled(0), inService(0)
		{}

		ubit32		flags;
		ubit32		nUnhandled;
		uarch_t		inService;
		/* Each IRQ __kpin has a list of installed ISRs. There is no
		 * limit to the number of ISRs which may be installed on a
		 * __kpin, but the kernel will eventually be equipped with the
		 * ability to balance the number of IRQs mapped to the same pin.
		 **/
		ptrListC<isrDescriptorS>	isrList;
	};

	// Array of vector descriptors. See above.
	vectorDescriptorS	msiIrqTable[ARCH_INTERRUPTS_NVECTORS];
	// Dynamic array of __kpin descriptors. See above.
	hardwareIdListC		pinIrqTable;
	// Counter for allocating __kpin IDs.
	ubit16			pinIrqTableCounter;
};

extern interruptTribC		interruptTrib;
extern "C" void interruptTrib_interruptEntry(registerContextC *regs);

#endif

