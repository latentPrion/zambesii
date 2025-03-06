#ifndef _SCALING_H
	#define _SCALING_H

	#include <config.h>

#define SCALING_UNI_PROCESSOR		0
#define SCALING_SMP			1
#define SCALING_CC_NUMA			2
#define SCALING_NONCC_NUMA		3

#ifdef CONFIG_CC_NUMA
	#define __SCALING__			SCALING_CC_NUMA
#elif defined( CONFIG_SMP )
	#define __SCALING__			SCALING_SMP
#elif defined( CONFIG_UNI_PROCESSOR )
	#define __SCALING__			SCALING_UNI_PROCESSOR
#else
	#error "No scaling mode selected. Please re-run kernel configuration with --enable-scaling=<up|smp|numa>"
#endif

#endif

