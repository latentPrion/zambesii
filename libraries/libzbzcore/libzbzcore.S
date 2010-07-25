#include <config.h>

/**	EXPLANATION:
 * This is the switchboard file for the syscall lib which is embedded into
 * the Endo-Kernel FS.
 *
 * As is to be expected, it should contain different code for each architecture.
 *
 * This is achieved through means of simply including the config.h file, and
 * analysing it to see which arch the kernel core was compiled for, then
 * including the right code here to be assembled.
 **/

#ifdef CONFIG_ARCH_x86_32
	#include "x8632/libzbzcore.S"
#else
	#error "No known architecture chosen in configure script. Try --arch=Myarch."
#endif

