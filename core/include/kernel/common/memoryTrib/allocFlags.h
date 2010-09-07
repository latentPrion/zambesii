#ifndef _MEMORY_ALLOCATION_FLAGS_H
	#define _MEMORY_ALLOCATION_FLAGS_H

#define MEMALLOC_NO_FAKEMAP			(1<<0)
#define MEMALLOC_PURE_VIRTUAL			(1<<1)
#define MEMALLOC_NO_SLEEP			(1<<2)
#define MEMALLOC_NO_FLUSH_ALL			(1<<3)
#define MEMALLOC_NO_FLUSH_RESERVOIR		(1<<4)
#define MEMALLOC_NO_FLUSH_PTCACHE		(1<<5)
#define MEMALLOC_NO_FLUSH_BANK_CACHE		(1<<6)

#endif
