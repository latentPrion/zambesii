#ifndef _PLATFORM_PAGE_TABLES_H
	#define _PLATFORM_PAGE_TABLES_H

	#include <arch/paging.h>

extern "C" sPagingLevel0	__kpagingLevel0Tables;
extern "C" sPagingLevel1	__kpagingLevel1Tables[];

#endif

