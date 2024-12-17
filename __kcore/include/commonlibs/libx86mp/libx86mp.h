#ifndef _x86_MP_LIBRARY_H
	#define _x86_MP_LIBRARY_H

	#include <__kstdlib/__ktypes.h>
	#include "mpTables.h"
	#include "lapic.h"
	#include "ioApic.h"

/**	EXPLANATION:
 * Stateful library which scans and parses x86 MP tables.
 *
 * The library is stateful in that it caches information. That is, when it
 * finds an MP Floating Pointer, for example, it caches that pointer's location,
 * so subsequent calls to the library do not need to rescan for an MP FP.
 **/


#endif

