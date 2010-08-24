
#include <__kstdlib/__kcxxlib/new>


static void *foo(void)
{
	return 0;
}

void *operator new (size_t)
{
	return foo();
}
void *operator new[](size_t)
{
	return foo();
}

void operator delete (void *)
{
}
void operator delete[](void *)
{
}

