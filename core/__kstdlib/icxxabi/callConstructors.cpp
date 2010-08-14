
#include <__ksymbols.h>
#include <__kstdlib/compiler/cxxrtl.h>

status_t cxxrtl::callGlobalConstructors(void)
{
	void (**ctorPtr)();

	ctorPtr = reinterpret_cast<void (**)()>( &__kctorStart );	
	for (;
		ctorPtr < reinterpret_cast<void (**)()>( &__kctorEnd );
		ctorPtr = reinterpret_cast<void (**)()>(
			(uarch_t)ctorPtr + sizeof(void *) ))
	{
		(*ctorPtr[0])();
	};

	return ERROR_SUCCESS;
}

