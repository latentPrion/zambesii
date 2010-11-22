
#include <chipset/chipset.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kstdlib/__kcxxlib/new>
#include <kernel/common/process.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/vfsTrib/vfsTrib.h>


processC::processC(processId_t processId)
:
id(processId)
{
	if (initMagic == PROCESS_INIT_MAGIC) {
		return;
	};

	memset(tasks, 0, sizeof(tasks));

	absName = argString = env = __KNULL;
	memoryStream = __KNULL;
}

error_t processC::initialize(utf8Char *fileAbsName)
{
	ubit32		nameLen;
	error_t		ret;

	nameLen = strlen8(fileAbsName);
	absName = new utf8Char[nameLen + 1];
	if (absName == __KNULL) {
		return ERROR_MEMORY_NOMEM;
	};

	// Initialize cpuTrace to the number of bits in Memory Trib bmp.
	ret = cpuTrace.initialize(cpuTrib.onlineCpus.getNBits());
	if (ret != ERROR_SUCCESS) {
		return ret;
	};

	// Allocate Memory Stream.
	memoryStream = new memoryStreamC(
		id,
		reinterpret_cast<void *>( 0x1000 ), 0x80000000, __KNULL,
		__KNULL, static_cast<paddr_t>( 0 ));

	if (memoryStream == __KNULL) {
		return ERROR_MEMORY_NOMEM;
	};

	return ERROR_SUCCESS;
}

processC::~processC(void)
{
	if (absName != __KNULL) {
		delete absName;
	};
	if (memoryStream != __KNULL) {
		delete memoryStream;
	};
}

