#ifdef __GNUC__
	// For GCC 4.5+, stdint is different, and the old one won't work.
	#if ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 5))
		#include "gcc-stdint-4.5.h"
	#else
		#include "gcc-stdint-4.4.h"
	#endif
#else
	#error "No stdint.h found for your compiler. Please provide one."
#endif

