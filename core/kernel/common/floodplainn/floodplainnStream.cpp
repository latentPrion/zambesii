
#include <kernel/common/floodplainn/floodplainnStream.h>


error_t FloodplainnStream::initialize(void)
{
	error_t			ret;

	ret = metaConnections.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

	ret = zkcmConnections.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

	return ERROR_SUCCESS;
}
