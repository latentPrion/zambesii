
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kclib/string16.h>
#include <__kstdlib/__kclib/stdlib.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <kernel/common/vfsTrib/vfsTrib.h>


vfsTribC::vfsTribC(void)
{
	trees.rsrc.arr = __KNULL;
	trees.rsrc.nTrees = 0;
}

vfsTribC::~vfsTribC(void)
{
}

void vfsTribC::dumpTrees(void)
{
	trees.lock.acquire();

	__kprintf(NOTICE VFSTRIB"Dumping.\t%d trees.\n", trees.rsrc.nTrees);
	for (uarch_t i=0; i<trees.rsrc.nTrees; i++)
	{
		__kprintf(NOTICE VFSTRIB"Tree %d: name: %[s]\n\tDesc @0x%p.\n",
			i, trees.rsrc.arr[i].name, trees.rsrc.arr[i].desc);
	};

	trees.lock.release();
}

vfsDirC *vfsTribC::createTree(utf16Char *name, uarch_t)
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

	// Allocate enough space to hold current trees + 1.
	tmp = static_cast<vfsDirC *>(
		malloc(sizeof(vfsDirC) * (trees.rsrc.nTrees + 1)) );

	if (tmp == __KNULL)
	{
		trees.lock.release();
		return __KNULL;
	};

	// Copy old array & use placement new() to construct new object.
	memcpy(tmp, trees.rsrc.arr, sizeof(vfsDirC) * trees.rsrc.nTrees);

	tmp2 = new (&(tmp[trees.rsrc.nTrees])) vfsDirC;
	if (tmp[trees.rsrc.nTrees].initialize() != ERROR_SUCCESS)
	{
		tmp[trees.rsrc.nTrees].~vfsDirC();
		free(tmp);

		trees.lock.release();
		return __KNULL;
	};

	strcpy16(tmp[trees.rsrc.nTrees].name, name);

	tmp2 = trees.rsrc.arr;
	trees.rsrc.arr = tmp;
	trees.rsrc.nTrees++;

	trees.lock.release();
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

error_t vfsTribC::setDefaultTree(utf16Char *name)
{
	vfsDirC		tmp;

	trees.lock.acquire();

	for (uarch_t i=0; i<trees.rsrc.nTrees; i++)
	{
		// If tree is found:
		if (strcmp16(trees.rsrc.arr[i].name, name) == 0)
		{
			memcpy(&tmp, &trees.rsrc.arr[i], sizeof(vfsDirC));
			memcpy(
				&trees.rsrc.arr[i], &trees.rsrc.arr[0],
				sizeof(vfsDirC));

			memcpy(&trees.rsrc.arr[0], &tmp, sizeof(vfsDirC));

			trees.lock.release();
			return ERROR_SUCCESS;
		};
	};

	trees.lock.release();
	return ERROR_INVALID_ARG_VAL;
}

