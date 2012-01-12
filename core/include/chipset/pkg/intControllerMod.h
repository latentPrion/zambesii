#ifndef _CHIPSET_INTERRUPT_CONTROLLER_MODULE_H
	#define _CHIPSET_INTERRUPT_CONTROLLER_MODULE_H

	#include <__kstdlib/__ktypes.h>

/**	EXPLANATION:
 * Interrupt Controller API.
 *
 * initialize():	Prepare the controller to receive commands from the
 *			kernel.
 * shutdown():		Kernel uses this to notify the controller that its
 *			services are no longer needed.
 * suspend():		When this is called, the device is to be placed into a
 *			state where, on kernel wake, the controller will not
 *			"surprise" the kernel with anything undue until the
 *			kernel calls the controller's awake() function.
 * awake():		Kernel uses this to notify the device that it is going
 *			into a reduced power state.
 * maskAll():		Mask off all IRQs at the controller.
 * maskSingle():	Mask off a the IRQ that pertains to a particular vector.
 * unmaskAll():		Unmask (enable) all IRQs at the controller.
 * unmaskSingle():	Unmask (enable) the IRQ that pertains to a particular
 *			vector.
 *
 * Of course, when the kernel passes a vector ID, it is the ID of the vector
 * from the kernel's perspective: that is, it is the CPU vector on which the
 * *kernel* sees this device's IRQs. So if a device's IRQ is remapped in
 * hardware, etc, such that it comes in on vector X on the CPU, the kernel only
 * knows that the device is functional on that vector. The controller is
 * responsible for "knowing" all of the hardware remapping details, and masking
 * off the right IRQ.
 **/

struct intControllerModS
{
	error_t (*initialize)(void);
	error_t (*shutdown)(void);
	error_t (*suspend)(void);
	error_t (*restore)(void);

	void (*maskAll)(void);
	error_t (*maskSingle)(uarch_t vector);
	void (*unmaskAll)(void);
	error_t (*unmaskSingle)(uarch_t vector);
	void (*sendEoi)(ubit8);
};

#endif

