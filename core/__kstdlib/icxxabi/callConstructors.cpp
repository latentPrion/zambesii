
#include <__ksymbols.h>
#include <__kstdlib/compiler/cxxrtl.h>

status_t cxxrtl::callGlobalConstructors(void)
{
	void (**ctorPtr)();

	ctorPtr = reinterpret_cast<void (**)()>( &__kctorStart );
	for (uarch_t i=0; (void *)ctorPtr < (void *)&__kctorEnd; i++) {
		(*ctorPtr[i])();
	};

	return ERROR_SUCCESS;
}

