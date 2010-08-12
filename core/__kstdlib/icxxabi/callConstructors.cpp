
#include <__ksymbols.h>
#include <__kstdlib/compiler/cxxrtl.h>

void cxxrtl::callGlobalConstructors(void)
{
	ctorPtr = reinterpret_cast<void (**)()>( &__kctorStart );
	for (; ctorPtr < reinterpret_cast<void(**)()>( &__kctorEnd );
		ctorPtr++)
	{
		(**ctorPtr)();
	};
}

