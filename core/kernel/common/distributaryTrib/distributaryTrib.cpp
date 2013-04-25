
#include <__kstdlib/__kclib/assert.h>
#include <kernel/common/panic.h>
#include <kernel/common/distributaryTrib/distributaryTrib.h>


error_t distributaryTribC::initialize(void)
{
	/**	EXPLANATION:
	 * Get the slam cache, etc initialized.
	 **/
	// Important in case we make changes and this goes unnoticed.
	assert_fatal(sizeof(distributaryTagC) == sizeof(categoryTagC));

	dtribTagCache = cachePool.createCache(sizeof(distributaryTagC));
	if (dtribTagCache == __KNULL)
	{
		__kprintf(ERROR DTRIBTRIB"Failed to create object cache.\n");
		return ERROR_UNKNOWN;
	};

	return rootCategory.initialize();
	// Build the tree here.
}

