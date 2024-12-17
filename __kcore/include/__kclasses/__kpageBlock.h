#ifndef ___KPAGE_BLOCK_H
	#define ___KPAGE_BLOCK_H

	#include <arch/paging.h>

#define PAGEBLOCK_NENTRIES(__type)				\
	((PAGING_BASE_SIZE - sizeof(typename __kPageBlock<T>::sHeader)) \
		/sizeof(__type))

template <class T>
class __kPageBlock
{
public:
	struct sHeader
	{
		__kPageBlock<T>	*next;
		// ubit32: We assume page sizes won't be reaching 4GB soon.
		ubit32			nFreeEntries;
	} header;
	T	entries[PAGEBLOCK_NENTRIES(T)];
};

#endif

