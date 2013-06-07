
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/assert.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/cachePool.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/panic.h>
#include <kernel/common/distributaryTrib/distributaryTrib.h>
#include <kernel/common/vfsTrib/vfsTrib.h>


error_t distributaryTribC::initialize(void)
{
	assert_fatal(
		sizeof(dvfs::distributaryTagC) == sizeof(dvfs::categoryTagC));

	distributaryTagCache = categoryTagCache = cachePool.createCache(
		sizeof(dvfs::distributaryTagC));

	if (!(distributaryTagCache != __KNULL && categoryTagCache != __KNULL)) {
		return ERROR_UNKNOWN;
	};

	return bootBuildTree();
}

error_t distributaryTribC::bootBuildTree(void)
{
	const dvfs::distributaryDescriptorS	*currDesc;
	uarch_t					i;
	dvfs::categoryTagC			*rootTag;
	error_t					ret;

	rootTag = vfsTrib.dvfsCurrentt.getRoot();
	assert_fatal(rootTag != __KNULL);
	assert_fatal(rootTag->getInode() != __KNULL);

	for (i=0, currDesc = distributaryDescriptors[i];
		currDesc != __KNULL;
		i++, currDesc = distributaryDescriptors[i])
	{
		sarch_t				inodeWasUsed=0;
		dvfs::distributaryInodeC	*dInodeTmp;

		__kprintf(NOTICE DTRIBTRIB"Desc: name \"%s\", vendor \"%s\", "
			"%d categories.\n"
			"\tdescription: \"%s\".\n",
			currDesc->name, currDesc->vendor,
			currDesc->nCategories,
			currDesc->description);

		dInodeTmp = new dvfs::distributaryInodeC(
			currDesc, dvfs::distributaryInodeC::IN_KERNEL);

		if (dInodeTmp == __KNULL)
		{
			__kprintf(NOTICE DTRIBTRIB"Failed to create dtrib "
				"inode for \"%s\".\n",
				currDesc->name);

			continue;
		};

		for (uarch_t j=0; j<currDesc->nCategories; j++)
		{
			dvfs::categoryTagC		*cTagTmp;
			dvfs::categoryInodeC		*cInodeTmp;
			dvfs::distributaryTagC		*dTagTmp;

			__kprintf(CC"\tCategory: %s.",
				currDesc->categories[j].name);

			// See if the category container exists already.
			cTagTmp = rootTag->getInode()->getDirTag(
				currDesc->categories[j].name);

			// If not, create it, and an inode for it.
			if (cTagTmp == __KNULL)
			{
				cInodeTmp = new dvfs::categoryInodeC;
				if (cInodeTmp == __KNULL) { continue; };
				if (cInodeTmp->initialize() != ERROR_SUCCESS)
					{ continue; };

				cTagTmp = rootTag->getInode()->createDirTag(
					currDesc->categories[j].name, rootTag,
					cInodeTmp, &ret);

				if (cTagTmp == __KNULL)
				{
					__kprintf(CC" Creation failed.\n");
					delete cInodeTmp;
					continue;
				};

				__kprintf(CC" Done.\n");
			}
			else
			{
				__kprintf(CC" Already exists.\n");

				/* Check to ensure that the distributary isn't
				 * being added to the same category twice.
				 **/
				dTagTmp = cTagTmp->getInode()->getLeafTag(
					currDesc->name);

				if (dTagTmp != __KNULL)
				{
					panic(FATAL"Distributary cannot have "
						"the same category listed "
						"more than once.\n"
						"\tDistributary category "
						"duplication detected.\n");
				};
			};

			// Now add the distributary to the category.
			dTagTmp = cTagTmp->getInode()->createLeafTag(
				currDesc->name, cTagTmp, dInodeTmp, &ret);

			if (dTagTmp != __KNULL) {
				inodeWasUsed = 1;
			};

			// If the distributary is also default, link it as such.
			if (currDesc->categories[j].isDefault)
			{
				if (cTagTmp->getInode()->getLeafTag(
					CC"default") != __KNULL)
				{
					__kprintf(FATAL DTRIBTRIB"Distributary "
						"\"%s\" is default for "
						"category \"%s\",\n"
						"\tbut a default already "
						"exists for it.\n",
						currDesc->name,
						currDesc->categories[j].name);

					panic(CC"Distributary anomaly.\n");
				};

				dTagTmp = cTagTmp->getInode()->createLeafTag(
					CC"default", cTagTmp, dInodeTmp, &ret);

				if (dTagTmp == __KNULL)
				{
					__kprintf(ERROR DTRIBTRIB"Failed to "
						"set dtrib %s as default for "
						"category %s.\n",
						currDesc->name,
						currDesc->categories[j].name);
				}
				else
				{
					__kprintf(NOTICE DTRIBTRIB"Set dtrib "
						"%s as default for category %s."
						"\n", currDesc->name,
						currDesc->categories[j].name);
				};
			};
		};
	};

	return ERROR_SUCCESS;
}

