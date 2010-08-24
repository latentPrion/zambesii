
#include <__kstdlib/__kclib/stdlib.h>


static void *foo(void)
{
	return 0;
}

void *malloc(uarch_t)
{
	return foo();
}

void *realloc(void *, uarch_t)
{
	return __KNULL;
}

void *calloc(uarch_t, uarch_t)
{
	return __KNULL;
}

void free(void *)
{
}

