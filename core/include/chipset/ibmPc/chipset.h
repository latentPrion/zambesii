#ifndef _CHIPSET_H
	#define _CHIPSET_H
	//IBM-PC

#define CHIPSET_SHORT_STRING		"IBM PC"
#define CHIPSET_LONG_STRING		"IBM/Intel PC Motherboard chipset"
#define CHIPSET_PATH			ibmPc

// Implies the size of the global array of process pointers.
#define CHIPSET_MAX_NPROCESSES		(16384)
// The maximum number of threads per process.
#define CHIPSET_MAX_NTASKS		(256)

#endif

