#ifndef _MULTI_LAYER_HASH_H
	#define _MULTI_LAYER_HASH_H

	#include <arch/arch.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kclib/string.h>
	#include <__kstdlib/__kcxxlib/new>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>

/**	EXPLANATION:
 * The Multi-Layer Hash is used in the VFS to store descriptors. There are two
 * hashes in the VFS: the file Descriptor hash and the directory descriptor
 * hash. These correspond to "VFS inodes" in Linux, except that Zambezii
 * distinguishes between VFS File inodes and VFS Directory inodes.
 *
 * The hash always takes a 32-bit key, and will always split it up into 10 bits,
 * 10 bits, and 12 bits. The splits are taken to be indexes into hash layers,
 * which are levels of indirection in the tree which will eventually point to
 * data. The hash works just like a page-table hierarchy.
 *
 * To make sure that the hash can always be split up as 10 bits, 10 bits, and
 * 12 bits, we make the layer size such that it can always hold 1024 pointers.
 * On a 32-bit architecture with 32-bit pointers, this means that the layer
 * size must be 4096 bytes. For a 64-bit arch with 64-bit pointers, the layer
 * size must be 8192 bytes, and so on.
 **/

#ifdef __32_BIT__
	#define MLHASH_LAYER_SIZE	(4096)
#elif defined(__64_BIT__)
	#define MLHASH_LAYER_SIZE	(8192)
#endif

#define MLHASH_PTRS_PER_LAYER		(MLHASH_LAYER_SIZE / sizeof(void *))
#define MLHASH_NLAYERS			(3)

#define MLHASH_GET_IDX1()	((key & 0xFFC00000) >> 22)
#define MLHASH_GET_IDX2()	((key & 0x003FF000) >> 12)
#define MLHASH_GET_IDX3()	(key & 0x00000FFF)

template <class T>
class multiLayerHashC
{
public:
	multiLayerHashC(void);
	error_t initialize(void);

public:
	error_t insert(ubit32 key, T *value);
	error_t get(ubit32 key, T **ret);

private:
	union mlHashLayerU
	{
		T	*leaf[MLHASH_PTRS_PER_LAYER];
		mlHashLayerU	*layer[MLHASH_PTRS_PER_LAYER];
	};

	sharedResourceGroupC<waitLockC, mlHashLayerU *>	top;

private:
	mlHashLayerU *getNewLayer(void);
};


/**	Template Definition
 *****************************************************************************/

template <class T>
multiLayerHashC<T>::multiLayerHashC(void)
{
	top.rsrc = 0;
}

template <class T>
error_t multiLayerHashC<T>::initialize(void)
{
	top.rsrc = getNewLayer();
	if (top.rsrc == __KNULL) {
		return ERROR_MEMORY_NOMEM;
	};
	return ERROR_SUCCESS;
}

template <class T>
multiLayerHashC<T>::mlHashLayerU *multiLayerHashC::getNewLayer(void)
{
	mlHashLayerU		*ret;

	ret = new mlHashLayerU;
	if (ret == __KNULL) {
		return __KNULL;
	};

	memset(ret, 0, sizeof(mlHashLayerU));
	return ret;
}

template <class T>
error_t multiLayerHashC<T>::insert(ubit32 key, T *item)
{
	ubit16		idx[MLHASH_NLAYERS];
	mlHashLayerU	*curLayer;

	idx[0] = MLHASH_GET_IDX1();
	idx[1] = MLHASH_GET_IDX2();
	idx[2] = MLHASH_GET_IDX3();

	top.lock.acquire()

	curLayer = top.rsrc;
	for (uarch_t i=0; i<MLHASH_NLAYERS; i++)
	{
		// If we're at a non-leaf layer:
		if (i < 2)
		{
			if (curLayer->layer[idx[i]] == __KNULL)
			{
				curLayer->layer[idx[i]] = getNewLayer();
				if (curLayer->layer[idx[i]] == __KNULL)
				{
					top.lock.release();
					return ERROR_MEMORY_NOMEM;
				};
			};

			curLayer = curLayer->layer[idx[i]];
			continue;
		};

		// Now we're at a leaf layer. Insert the new item.
		curLayer->leaf[idx[i]] = item;
	};

	top.lock.release();
	return ERROR_SUCCESS;
}

template <class T>
error_t multiLayerHashC<T>::get(ubit32 key, T **ret)
{
	ubit16		idx[MLHASH_NLAYERS];
	mlHashLayerU	*curLayer;

	idx[0] = MLHASH_GET_IDX1();
	idx[1] = MLHASH_GET_IDX2();
	idx[2] = MLHASH_GET_IDX3();

	top.lock.acquire();

	curLayer = top.rsrc;
	for (uarch_t i=0; i<MLHASH_NLAYERS; i++)
	{
		// If we're at a non-leaf layer:
		if (i < 2)
		{
			// If tree doesn't go that far down,
			if (curLayer->layer[idx[i]] == __KNULL)
			{
				top.lock.release();
				// Item doesn't exist.
				return ERROR_GENERAL;
			};
			curLayer = curLayer->layer[idx[i]];
			continue;
		};

		// Now we're at a leaf layer. Get return value.
		*ret = curLayer->leaf[idx[i]];
	};

	top.lock.release();
	return ERROR_SUCCESS;
}

#endif

