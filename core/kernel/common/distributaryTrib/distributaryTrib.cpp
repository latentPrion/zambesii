
#include <__kstdlib/__kclib/assert.h>
#include <__kstdlib/__kclib/string8.h>
#include <kernel/common/panic.h>
#include <kernel/common/distributaryTrib/distributaryTrib.h>


distributaryTribC::distributaryDescriptorS::distributaryDescriptorS(
	distributaryDescriptorS &source
	)
:
majorVersion(source.majorVersion), minorVersion(source.minorVersion),
patchVersion(source.patchVersion),
entryAddress(source.entryAddress)
{
	strncpy8(category, source.category, DTRIBTRIB_TAG_NAME_MAX_NCHARS);
	strncpy8(name, source.name, DTRIBTRIB_TAG_NAME_MAX_NCHARS);
	strncpy8(vendor, source.vendor, sizeof(vendor));
	strncpy8(description, source.description, sizeof(description));
}

error_t distributaryTribC::initialize(void)
{
	error_t		ret;

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

	ret = rootCategory.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

	buildTree();
	return ERROR_SUCCESS;
}

void distributaryTribC::buildTree(void)
{
	__kprintf(NOTICE DTRIBTRIB"buildTree: building VFS tree.\n");
}

