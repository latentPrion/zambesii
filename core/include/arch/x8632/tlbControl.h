
inline void tlbControl::flushSingleEntry(void *vaddr)
{
	asm volatile (
		"invlpg (%0) \n\t"
		: "=r" (vaddr)
	);
}

