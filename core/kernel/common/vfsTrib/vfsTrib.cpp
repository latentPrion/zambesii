
#include <__kstdlib/__kclib/stdlib.h>
#include <__kstdlib/__kcxxlib/new>
#include <kernel/common/vfsTrib/vfsTrib.h>


vfsTribC::vfsTribC(void)
{
	trees.rsrc.arr = __KNULL;
	trees.rsrc.nTrees = 0;
}

vfsTribC::~vfsTribC(void)
{
}

vfsDirC *vfsTribC::createTree(utf16Char *name, uarch_t flags)
{
	vfsDirC		*tmp, *tmp2;

	/**	EXPLANATION:
	 * Note carefully that fact that when allocating the new array in 'tmp'
	 * I don't use new(), but malloc() so that the array's objects won't
	 * waste time with construction.
	 *
	 * Also, when freeing the old array after resizing, I don't use
	 * delete(), but free(), after explicitly calling the destructor for the
	 * specific object I wish to destroy. Using delete() would destruct all
	 * of the tree objects in the array, and that's not what we want.
	 **/

	// First search the current tree set.
	trees.lock.acquire();

	for (uarch_t i=0; i<trees.rsrc.nTrees; i++)
	{
		// If tree with this name already exists:
		if (strcmp16(trees.rsrc.arr[i].name, name) == 0)
		{
			trees.lock.release();
			return &trees.rsrc.arr[i];
		};
	};

	// Create the tree. FIXME: Allocation within a locked section.
	tmp = malloc(sizeof(vfsDirC) * (trees.rsrc.nTrees + 1));
	if (tmp == __KNULL)
	{
		trees.lock.release();
		return __KNULL;
	};

	// Copy old array & use placement new() to construct new object.
	memcpy(tmp, trees.rsrc.arr, sizeof(vfsDirC) * trees.rsrc.nTrees);

	tmp = new (&tmp[trees.rsrc.nTrees]) vfsDirC;
	if (tmp[trees.rsrc.nTrees].initialize() != ERROR_SUCCESS)
	{
		tmp[trees.rsrc.nTree].~vfsDirC();
		free(tmp);

		trees.lock.release();
		return __KNULL;
	};

	tmp2 = trees.rsrc.arr;
	trees.rsrc.arr = tmp;
	trees.rsrc.nTrees++;

	tree.lock.release();
	free(tmp2);
	return tmp;
}

error_t vfsTribC::deleteTree(utf16Char *name)
{
	trees.lock.acquire();

	for (uarch_t i=0; i<trees.rsrc.nTrees; i++)
	{
		// If tree is found:
		if (strcmp16(trees.rsrc.arr[i].name, name) == 0)
		{
			// Run destructor, then erase.
			trees.rsrc.arr[i].~vfsDirC();
			memcpy(
				&trees.rsrc.arr[i], &trees.rsrc.arr[i+1],
				sizeof(vfsDirC) * (trees.rsrc.nTrees - i - 1));

			trees.rsrc.nTrees--;

			trees.lock.release();
			return ERROR_SUCCESS;
		};
	};

	return ERROR_INVALID_ARG_VAL;
}


