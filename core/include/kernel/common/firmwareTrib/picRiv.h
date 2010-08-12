#ifndef _PIC_RIVULET_H
	#define _PIC_RIVULET_H

	#include <__kstdlib/__ktypes.h>

/**	EXPLANATION:
 * A call to awake() is expected to place the chipset in such a state that
 * when the kernel awakens, the chipset will not bother it with distracting
 * nonsense.
 **/

struct picRivS
{
	error_t (*initialize)(void);
	error_t (*shutdown)(void);
	error_t (*suspend)(void);
	error_t (*awake)(void);

	void (*maskAll)(void);
	error_t (*maskSingle)(uarch_t vector);
	void (*unmaskAll)(void);
	error_t (*unmaskSingle)(uarch_t vector);
};

#endif

