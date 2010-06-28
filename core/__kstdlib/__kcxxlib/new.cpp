
#include <__kstdlib/__kcxxlib/new>

/**	EXPLANATION:
 * The Zambezii pool allocator.
 *
 * The Zambezii pool allocator is a simple heap manager for the kernel which
 * provides buffered dynamic memory allocation by partitioning off the blocks
 * given off by the kernel.
 *
 * The details are still unknown, since I haven't yet found a way to properly
 * allocate objects of any size which are relatively free of any waste
 * potential.
 *
 * I *DO* have one method in mind which is EXTREMELY efficient, but I'll think
 * on it more before actually implementing it.
 *
 * Note well that operator new() implicitly allocates as a C/C++ std lib, and
 * uses buffered memory given by the kernel. I think we should provide two
 * userspace C/C++ libraries. The first will allocate objects using our
 * specialized allocator, and the second will allocate using a general
 * allocator.
 **/

static void *foo() { return __KNULL; }

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

