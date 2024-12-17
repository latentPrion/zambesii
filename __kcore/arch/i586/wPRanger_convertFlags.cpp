
#include <arch/walkerPageRanger.h>
#include <__kstdlib/__kflagManipulation.h>

uarch_t walkerPageRanger::encodeFlags(uarch_t flags)
{
	uarch_t		ret=0;

#ifdef CONFIG_ARCH_x86_32_PAE
	if (!FLAG_TEST(flags, PAGEATTRIB_EXECUTABLE)) {
		ret |= PAGING_L2_NO_EXECUTE;
	};
	if (FLAG_TEST(flags, PAGEATTRIB_WRITE)) {
		ret |= PAGING_L2_WRITE;
	};
	if (!FLAG_TEST(flags, PAGEATTRIB_SUPERVISOR)) {
		ret |= PAGING_L2_USER;
	};
	if (FLAG_TEST(flags, PAGEATTRIB_PRESENT)) {
		ret |= PAGING_L2_PRESENT;
	};
	if (FLAG_TEST(flags, PAGEATTRIB_TLB_GLOBAL)) {
		ret |= PAGING_L2_TLB_GLOBAL;
	};
	if (FLAG_TEST(flags, PAGEATTRIB_CACHE_WRITE_THROUGH)) {
		ret |= PAGING_L2_CACHE_WRITE_THROUGH;
	};
	if (FLAG_TEST_(flags, PAGEATTRIB_CACHE_DISABLED)) {
		ret |= PAGING_L2_CACHE_DISABLED;
	};
#else
	if (FLAG_TEST(flags, PAGEATTRIB_WRITE)) {
		ret |= PAGING_L1_WRITE;
	};
	if (!FLAG_TEST(flags, PAGEATTRIB_SUPERVISOR)) {
		ret |= PAGING_L1_USER;
	};
	if (FLAG_TEST(flags, PAGEATTRIB_PRESENT)) {
		ret |= PAGING_L1_PRESENT;
	};
	if (FLAG_TEST(flags, PAGEATTRIB_TLB_GLOBAL)) {
		ret |= PAGING_L1_TLB_GLOBAL;
	};
	if (FLAG_TEST(flags, PAGEATTRIB_CACHE_WRITE_THROUGH)) {
		ret |= PAGING_L1_CACHE_WRITE_THROUGH;
	};
	if (FLAG_TEST(flags, PAGEATTRIB_CACHE_DISABLED)) {
		ret |= PAGING_L1_CACHE_DISABLED;
	};
#endif
	return ret;
}

uarch_t walkerPageRanger::decodeFlags(uarch_t flags)
{
	uarch_t		ret=0;

#ifdef CONFIG_ARCH_x86_32_PAE
	if (!FLAG_TEST(flags, PAGING_L2_NO_EXECUTE)) {
		ret |= PAGEATTRIB_EXECUTABLE;
	};
	if (FLAG_TEST(flags, PAGING_L2_WRITE)) {
		ret |= PAGEATTRIB_WRITE;
	};
	if (!FLAG_TEST(flags, PAGING_L2_USER)) {
		ret |= PAGEATTRIB_SUPERVISOR;
	};
	if (FLAG_TEST(flags, PAGING_L2_PRESENT)) {
		ret |= PAGEATTRIB_PRESENT;
	};
	if (FLAG_TEST(flags, PAGING_L2_TLB_GLOBAL)) {
		ret |= PAGEATTRIB_TLB_GLOBAL;
	};
	if (FLAG_TEST(flags, PAGING_L2_CACHE_WRITE_THROUGH)) {
		ret |= PAGEATTRIB_CACHE_WRITE_THROUGH;
	};
	if (FLAG_TEST(flags, PAGING_L2_CACHE_DISABLED)) {
		ret |= PAGEATTRIB_CACHE_DISABLED;
	};
#else
	if (FLAG_TEST(flags, PAGING_L1_WRITE)) {
		ret |= PAGEATTRIB_WRITE;
	};
	if (!FLAG_TEST(flags, PAGING_L1_USER)) {
		ret |= PAGEATTRIB_SUPERVISOR;
	};
	if (FLAG_TEST(flags, PAGING_L1_PRESENT)) {
		ret |= PAGEATTRIB_PRESENT;
	};
	if (FLAG_TEST(flags, PAGING_L1_TLB_GLOBAL)){
		ret |= PAGEATTRIB_TLB_GLOBAL;
	};
	if (FLAG_TEST(flags, PAGING_L1_CACHE_WRITE_THROUGH)) {
		ret |= PAGEATTRIB_CACHE_WRITE_THROUGH;
	};
	if (FLAG_TEST(flags, PAGING_L1_CACHE_DISABLED)) {
		ret |= PAGEATTRIB_CACHE_DISABLED;
	};
#endif
	return ret;
}

