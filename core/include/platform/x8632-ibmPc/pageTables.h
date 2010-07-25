#ifndef _PLATFORM_PAGE_TABLES_H
	#define _PLATFORM_PAGE_TABLES_H

extern "C" pagingLevel0S	*__kpagingLevel0Tables;
extern "C" pagingLevel1S	*__kpagingIdMapTables, *__kpagingLevel1Tables;
extern "C" pagingLevel1S	*__kpagingL1ModifierTables;

#endif

