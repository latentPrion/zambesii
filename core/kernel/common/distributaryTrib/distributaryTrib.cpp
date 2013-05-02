
#include <__kstdlib/__kclib/assert.h>
#include <__kstdlib/__kclib/string8.h>
#include <kernel/common/panic.h>
#include <kernel/common/distributaryTrib/distributaryTrib.h>


distributaryTribC::distributaryInodeC::distributaryInodeC(
	distributaryDescriptorS &source
	)
{
	majorVersion = source.majorVersion;
	minorVersion = source.minorVersion;
	patchVersion = source.patchVersion;
	entryAddress = source.entryAddress;

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
	categoryInodeC			*root;
	categoryTagC			*cat;
	distributaryTagC		*dtrib;
	const distributaryDescriptorS	*curr;
	uarch_t				i;

	root = getRootCategory();
	curr = distributaryDescriptors[0];
	for (i=0;
		curr != __KNULL;
		i++, curr = distributaryDescriptors[i])
	{
		cat = root->getCategory(distributaryDescriptors[i]->category);
		if (cat == __KNULL)
		{
			cat = root->createCategory(
				distributaryDescriptors[i]->category);

			if (cat == __KNULL)
			{
				__kprintf(ERROR DTRIBTRIB"buildTree: Failed "
					"to create category \"%s\".\n",
					distributaryDescriptors[i]->category);

				continue;
			};
		};

		dtrib = cat->getInode()->createDistributaryTag(
			distributaryDescriptors[i]->name);

		if (dtrib == __KNULL)
		{
			__kprintf(ERROR DTRIBTRIB"buildTree: Failed to create "
				"distributary for \"%s\".\n",
				distributaryDescriptors[i]->name);

			continue;
		};

		__kprintf(NOTICE DTRIBTRIB"buildTree: Created distributary "
			"\"%s\".\n",
			dtrib->getName());
	};

	__kprintf(NOTICE DTRIBTRIB"buildTree: Processed %d Distribs.\n", i);
}

