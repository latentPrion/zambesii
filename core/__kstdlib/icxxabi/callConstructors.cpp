
#include <debug.h>
#include <__ksymbols.h>
#include <__kstdlib/compiler/cxxrtl.h>
#include <__kclasses/cachePool.h>

status_t cxxrtl::callGlobalConstructors(void)
{
	void (**ctorPtr)();

	ctorPtr = reinterpret_cast<void (**)()>( &__kctorStart );
	for (uarch_t i=0;
		ctorPtr < reinterpret_cast<void (**)()>( &__kctorEnd );
		ctorPtr = reinterpret_cast<void (**)()>(
			(uarch_t)ctorPtr + sizeof(void *) ))
	{
		(*ctorPtr[0])();
		i++;

		if (rr == 3 && cachePool.debugCheck() != ERROR_SUCCESS)
		{
			asm volatile(
				"movl	%0, %%eax\n\t"
				"movl	%1, %%ebx\n\t"
				"movl	$0x2b201234, %%ecx\n\t"
				:
				: "r" (ctorPtr[0]), "r" (i)
				: "ebx", "eax");

			// c0007b8c
			for (;;);
		};
	};

	return ERROR_SUCCESS;
}

