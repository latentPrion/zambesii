
#include <__kstdlib/__kcxxlib/cstring>
#include <__kthreads/__korientation.h>
#include <__kthreads/__korientationPreConstruct.h>
#include <kernel/common/process.h>
#include <kernel/common/memoryTrib/memoryTrib.h>

processS	__kprocess;

void __korientationPreConstruct::__kprocessInit(void)
{
	memset(&__kprocess, 0, sizeof(processS));

	__kprocess.head = &__korientationThread;
	__kprocess.fileName = const_cast<char *>( "zambezii.zxe" );
	__kprocess.filePath = const_cast<char *>( "/zambezii/core" );
	// Probably set argString to the kernel command line later on.
	__kprocess.memoryStream = &memoryTrib.__kmemoryStream;
}

