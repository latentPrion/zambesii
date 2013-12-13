
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/assert.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/cachePool.h>
#include <__kclasses/debugPipe.h>
#include <__kclasses/singleWaiterQueue.h>
#include <kernel/common/process.h>
#include <kernel/common/thread.h>
#include <kernel/common/panic.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/distributaryTrib/distributaryTrib.h>
#include <kernel/common/vfsTrib/vfsTrib.h>


error_t distributaryTribC::initialize(void)
{
	tagCache = cachePool.createCache(sizeof(dvfs::tagC));
	if (tagCache == NULL) { return ERROR_UNKNOWN; };

	return bootBuildTree();
}

error_t distributaryTribC::bootBuildTree(void)
{
	const dvfs::distributaryDescriptorS	*currDesc;
	uarch_t					i;
	dvfs::tagC				*rootTag;
	error_t					ret;

	rootTag = vfsTrib.dvfsCurrentt.getRoot();
	assert_fatal(rootTag != NULL);
	assert_fatal(rootTag->getInode() != NULL);

	for (i=0, currDesc = distributaryDescriptors[i];
		currDesc != NULL;
		i++, currDesc = distributaryDescriptors[i])
	{
		sarch_t				inodeWasUsed=0;
		dvfs::distributaryInodeC	*dInodeTmp;

		printf(NOTICE DTRIBTRIB"Desc: name \"%s\", vendor \"%s\", "
			"%d categories.\n"
			"\tdescription: \"%s\".\n",
			currDesc->name, currDesc->vendor,
			currDesc->nCategories,
			currDesc->description);

		dInodeTmp = new dvfs::distributaryInodeC(
			currDesc, dvfs::distributaryInodeC::IN_KERNEL);

		if (dInodeTmp == NULL)
		{
			printf(NOTICE DTRIBTRIB"Failed to create dtrib "
				"inode for \"%s\".\n",
				currDesc->name);

			continue;
		};

		for (uarch_t j=0; j<currDesc->nCategories; j++)
		{
			dvfs::tagC		*cTagTmp, *dTagTmp;
			dvfs::categoryInodeC	*cInodeTmp;

			printf(CC"\tCategory: %s.",
				currDesc->categories[j].name);

			// See if the category container exists already.
			cTagTmp = rootTag->getCInode()->getDirTag(
				currDesc->categories[j].name);

			// If not, create it, and an inode for it.
			if (cTagTmp == NULL)
			{
				cInodeTmp = new dvfs::categoryInodeC;
				if (cInodeTmp == NULL) { continue; };
				if (cInodeTmp->initialize() != ERROR_SUCCESS)
					{ continue; };

				cTagTmp = rootTag->getCInode()->createDirTag(
						currDesc->categories[j].name,
						vfs::DIR,
						rootTag, cInodeTmp, &ret);

				if (cTagTmp == NULL)
				{
					printf(CC" Creation failed.\n");
					delete cInodeTmp;
					continue;
				};

				printf(CC" Done.\n");
			}
			else
			{
				printf(CC" Already exists.\n");

				/* Check to ensure that the distributary isn't
				 * being added to the same category twice.
				 **/
				dTagTmp = cTagTmp->getCInode()
					->getLeafTag(currDesc->name);

				if (dTagTmp != NULL)
				{
					panic(FATAL"Distributary cannot have "
						"the same category listed "
						"more than once.\n"
						"\tDistributary category "
						"duplication detected.\n");
				};
			};

			// Now add the distributary to the category.
			dTagTmp = cTagTmp->getCInode()->createLeafTag(
				currDesc->name, vfs::FILE,
				cTagTmp, dInodeTmp, &ret);

			if (dTagTmp != NULL) {
				inodeWasUsed = 1;
			};

			// If the distributary is also default, link it as such.
			if (currDesc->categories[j].isDefault)
			{
				if (cTagTmp->getCInode()->getLeafTag(
					CC"default") != NULL)
				{
					printf(FATAL DTRIBTRIB"Distributary "
						"\"%s\" is default for "
						"category \"%s\",\n"
						"\tbut a default already "
						"exists for it.\n",
						currDesc->name,
						currDesc->categories[j].name);

					panic(CC"Distributary anomaly.\n");
				};

				dTagTmp = cTagTmp->getCInode()->createLeafTag(
						CC"default", vfs::FILE,
						cTagTmp, dInodeTmp, &ret);

				if (dTagTmp == NULL)
				{
					printf(ERROR DTRIBTRIB"Failed to "
						"set dtrib %s as default for "
						"category %s.\n",
						currDesc->name,
						currDesc->categories[j].name);
				}
				else
				{
					printf(NOTICE DTRIBTRIB"Set dtrib "
						"%s as default for category %s."
						"\n", currDesc->name,
						currDesc->categories[j].name);
				};
			};
		};
	};

	return ERROR_SUCCESS;
}

