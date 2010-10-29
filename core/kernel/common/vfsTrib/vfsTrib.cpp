
#include <__kstdlib/__kcxxlib/new>
#include <kernel/common/vfsTrib/vfsTrib.h>


vfsTribC::createTree(utf16Char *name, uarch_t flags)
{
	vfsDirC		*cur, *tmp;

	// First search the current tree set.
	trees.lock.acquire();

	cur = trees.rsrc.arr;
	for (uarch_t i=0; i<trees.rsrc.nTrees; i++)
	{
		// If tree with this name already exists:
		if (strcmp16(cur->name, name) == 0)
		{
			trees.lock.release();
			return cur;
		};
	};

	// Resize the current array of trees.
	tmp = new vfsDirC[trees.rsrc.nTrees + 1];
	if (tmp == __KNULL)
	{
		trees.lock.release();
		return __KNULL;
	};

	memcpy(tmp, trees.rsrc.arr, sizeof(vfsDirC) * trees.rsrc.nTrees);
	strcpy16(tmp[trees.rsrc.nTrees].name, name);
	tmp[trees.rsrc.nTrees].flags = 0;
	tmp[trees.rsrc.nTrees].type = 0;

	tmp[trees.rsrc.nTrees].desc = new vfsDirDesC;
	if (tmp[trees.rsrc.nTrees].desc == __KNULL)
	{
		trees.lock.release();

		delete tmp;
		return __KNULL;
	};

	
	delete trees.rsrc.arr;
	trees.rsrc.arr = tmp;
	trees.rsrc.nTrees++;

	trees.lock.release();
	return tmp;
}


