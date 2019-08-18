#ifndef _ZBZ_DMA_H
	#define _ZBZ_DMA_H

	#define UDI_VERSION 0x101
	#define UDI_PHYSIO_VERSION 0x101
	#include <udi.h>
	#include <udi_physio.h>
	#undef UDI_VERSION
	#include <__kstdlib/__kclib/assert.h>
	#include <__kclasses/list.h>
	#include <__kclasses/resizeableArray.h>
	#include <kernel/common/thread.h>
	#include <kernel/common/memoryTrib/memoryTrib.h>
	#include <kernel/common/cpuTrib/cpuTrib.h>
	#include "scgth_t_paddr_t_operators.h"

#define SGLIST		"SGList: "

class FloodplainnStream;

namespace fplainn
{

namespace dma
{

#if __PADDR_NBITS__ > 32 && __PADDR_NBITS__ <= 64
typedef udi_scgth_element_64_t __kscgth_element_type_t;
#elif __PADDR_NBITS__ <= 32
typedef udi_scgth_element_32_t __kscgth_element_type_t;
#else
	#error "Cannot determine what element size __kscgth_element_type_t should use."
#endif

namespace scatterGatherLists
{
	void *getPages(Thread *t, uarch_t nPages);
	void releasePages(Thread *t, void *vaddr, uarch_t nPages);

	status_t wprMapInc(Thread *t, void *vaddr, paddr_t p, uarch_t nFrames);
	status_t wprUnmap(Thread *t, void *vaddr, uarch_t nFrames);
}

/**	EXPLANATION:
 * This class represents Zambesii's UDI DMA constraints feature.
 */
class Constraints;
namespace constraints
{

class Compiler
{
public:
	Compiler(void)
	{
		memset(&i, 0, sizeof(i));
		setInvalid();
		i.version = 1;
	}

	error_t initialize(void) { return ERROR_SUCCESS; };

	error_t compile(Constraints *cons);

	sbit8 isValid(void)
	{
		return (i.version > 0) && (i.minElementGranularityNFrames > 0);
	}

	void setInvalid(void)
	{
		i.version = 0;
		i.minElementGranularityNFrames = paddr_t(0);
	}

	void dump(void);

public:
	struct {
		/**	EXPLANATION:
		 * This is a version number for this struct layout.
		 * Additions to this struct layout will be appended after
		 * the members that make up previous version iterations.
		 */
		uarch_t		version;
		ubit8		callerSupportedFormats;
		sbit8		partialAllocationsDisallowed,
				sequentialAccessHint;
		ubit8		addressableBits,
				fixedBits;
		uarch_t		pfnSkipStride, maxElementsPerSegment;
		paddr_t		startPfn, beyondEndPfn,
				// minElemGranNFrames cannot be < 1.
				minElementGranularityNFrames,
				maxNContiguousFrames,
				slopInBits, slopOutBits,
				slopOutExtra, slopBarrierBits,
				fixedBitsValue;
	} i;

private:
};

}

/**	EXPLANATION:
 * This class encapsulates the ZBZ implementation of udi_buf_t for
 * reading/writing from/to a UDI DMA SGlist. SGLists are in physical memory, so
 * they must be mapped before their contents can be altered.
 *
 * This class provides methods such as memset(), memmove(), read(), write(),
 * etc.
 ******************************************************************************/
class ScatterGatherList;

class MappedScatterGatherList
{
public:
	MappedScatterGatherList(ScatterGatherList *_parent)
	:
	parent(_parent)
	{
		trackPages(NULL, 0);
	}

	error_t initialize(void)
	{
		return ERROR_SUCCESS;
	}

	~MappedScatterGatherList(void) {}

public:
	error_t trackPages(void *vaddr, uarch_t nPages);
	error_t removePages(void *vaddr, uarch_t nPages);
	void compact(void);

public:
	void			*vaddr;
	uarch_t			nPages;
	ScatterGatherList	*parent;
};

/**	EXPLANATION:
 * This is an abstraction around the udi_scgth_t type, meant to make the
 * building and manipulation of DMA elements easier.
 *
 * It also provides a map() method, which causes it to read all of its
 * scatter-gather elements and return a virtual mapping to them. The purpose of
 * this of course, is to enable the kernel to read/write the contents stored in
 * a particular scatter-gather list's memory.
 ******************************************************************************/
extern ubit8		nullSGList[];

class ScatterGatherList
{
	friend class ::FloodplainnStream;
public:
	ScatterGatherList(void)
	:
	/* ScatterGatherList instances are manipulated by the PMM, so their
	 * metadata must not be fakemapped.
	 */
	elements(RESIZEABLE_ARRAY_FLAGS_NO_FAKEMAP),
	mapping(this),
	isFrozen(0)
	{}

	error_t initialize(void)
	{
		error_t		ret;

		ret = elements.initialize();
		if (ret != ERROR_SUCCESS) { return ret; };

		ret = compiledConstraints.initialize();
		if (ret != ERROR_SUCCESS) { return ret; };

		ret = mapping.initialize();
		if (ret != ERROR_SUCCESS) { return ret; };

		return ERROR_SUCCESS;
	}

	~ScatterGatherList(void);

public:
	void dump(void);

	void lock(void)
	{
		elements.lock();
	}

	void unlock(void)
	{
		elements.unlock();
	}

	int operator ==(void *p)
	{
		assert_fatal(p == NULL);

		/**	EXPLANATION:
		 * This operator is implemented for the benefit of the
		 * comparison to NULL which is done against it in
		 * WrapAroundCounter.
		 **/
		return !memcmp(this, nullSGList, sizeof(*this));
	}

	error_t constrain(constraints::Compiler *compiledCon)
	{
		if (compiledCon == NULL)
		{
			printf(ERROR"SGList::constrain: compiledCon is "
				"NULL.\n");

			return ERROR_INVALID_ARG;
		}

		compiledConstraints = *compiledCon;
		return ERROR_SUCCESS;
	}

	enum preallocateEntriesE {
		PE_FLAGS_UNLOCKED		= (1<<0)
	};
	error_t preallocateEntries(uarch_t nEntries, uarch_t flags=0);

	enum addFramesE {
		AF_FLAGS_UNLOCKED		= (1<<0)
	};
	status_t addFrames(
		paddr_t p, uarch_t nFrames, sarch_t atIndex, uarch_t flags);

	enum getNFramesE {
		GNF_FLAGS_UNLOCKED		= (1<<0)
	};
	uarch_t getNFrames(uarch_t flags=0);

	status_t getElements(void *outarr, uarch_t nelem, ubit8 *outarrType);
	error_t resize(uarch_t nFrames);

	/* Returns the number of elements that were discarded
	 * due to compaction.
	 **/
	uarch_t compact(void);
	// Maps this SGList and records the page list using its MappedSGList.
	error_t map(void);
	// Destroys a mapping to this SGList.
	void unmap(void);
	// Remaps this SGList.
	error_t remap(void)
	{
		unmap();
		return map();
	}

	/**	EXPLANATION:
	 * This function should be renamed into something like iommuMap().
	 *
	 * We don't need to freeze SGLists -- we need only ensure that whe
	 * their frame list contents are changed, that we force the IOMM
	 * mappings to be made invalid such that the respective device can no
	 * longer access the frames which were previously in the list.
	 */
	status_t toggleFreeze(sbit8 freeze_or_unfreeze)
	{
		(void)freeze_or_unfreeze;
		printf(WARNING SGLIST"toggleFreeze: Unimplemented!\n");
		return ERROR_SUCCESS;
	}

private:
	void freeSGListElements(void)
	{
		typename SGListElementArray::Iterator		it;

		lock();
		for (it = elements.begin(); it != elements.end(); ++it)
		{
			__kscgth_element_type_t		*tmp = &*it;
			paddr_t				p;

			assign_scgth_block_busaddr_to_paddr(p, tmp->block_busaddr);

			memoryTrib.releaseFrames(p, tmp->block_length);
		}
		unlock();
	}

	void destroySGList(sbit8 justTransfer);

	/* This is the backend for
	 *	resize(uarch_t);
	 *
	 * This section had to be split off because we don't want to have to
	 * #include memoryTrib.h in this header.
	 **/
	status_t unlocked_resize(uarch_t nFrames);

public:
	typedef ResizeableArray<udi_scgth_element_32_t>
					SGList32;
	typedef ResizeableArray<udi_scgth_element_64_t>
					SGList64;

	typedef ResizeableArray<__kscgth_element_type_t>
					SGListElementArray;

	SGListElementArray			elements;
	constraints::Compiler			compiledConstraints;
	MappedScatterGatherList			mapping;
	// XXX: FIXME: This should be read/written with atomic operations.
	sarch_t					isFrozen;
};

class Constraints
{
public:
	friend class MemoryBmp;

	Constraints(void) {}

	error_t initialize(
		udi_dma_constraints_attr_spec_t *_attrs, uarch_t _nAttrs)
	{
		if (_attrs == NULL)
		{
			nAttrs = 0;
			return ERROR_INVALID_ARG;
		}

		nAttrs = _nAttrs;
		if (_nAttrs == 0) { return ERROR_SUCCESS; }

		memcpy(attrs, _attrs, sizeof(*_attrs) * _nAttrs);

		// Sanity check for holes.
		for (uarch_t i=0; i<nAttrs; i++)
		{
			if (isValidConstraintAttrType(attrs[i].attr_type))
			{
				continue;
			}

			memcpy(
				&attrs[i], &attrs[i+1],
				sizeof(*attrs) * (nAttrs - (i + 1)));
			nAttrs--;
		}

		return ERROR_SUCCESS;
	}

public:
	void dump(void);
	static utf8Char *getAttrTypeName(
		udi_dma_constraints_attr_t a);

	static sbit8 isValidConstraintAttrType(udi_dma_constraints_attr_t val);
	sbit8 attrAlreadySet(udi_dma_constraints_attr_t a)
	{
		for (uarch_t i=0; i<nAttrs; i++)
		{
			udi_dma_constraints_attr_spec_t *s = &attrs[i];

			if (s->attr_type != a) { continue; }
			return 1;
		};

		return 0;
	}

	/* We don't need to provide a "remove()" method since
	 * that is not allowed by UDI. Can only reset() single
	 * attributes.
	 **/
	error_t addOrModifyAttrs(
		udi_dma_constraints_attr_spec_t *attrs,
		uarch_t nAttrs);

	udi_dma_constraints_attr_spec_t *getAttr(
		udi_dma_constraints_attr_t attr);

public:
	#define N_ATTR_TYPE_NAMES	(32)
	static utf8Char			*attrTypeNames[N_ATTR_TYPE_NAMES];
	udi_dma_constraints_attr_spec_t attrs[N_ATTR_TYPE_NAMES];
	uarch_t				nAttrs;
};

/**	EXPLANATION:
 * This class represents a driver's request to perform a DMA transaction. It
 * directs the entire operation, from allocating the DMA SGList, to copying data
 * to and from DMA memory, and so on.
 *
 * This is what Zambesii's udi_dma_handle_t handle points to.
 *
 * An instance of this class is created each time a user calls
 * udi_dma_prepare(). There can be multiple instances of this class which all
 * refer to the same udi_dma_constraints_t userspace-object.
 *
 * Essentially, each Request instance represents a DMA transaction, and
 * multiple DMA transactions can have the same behavioural constraints.
 *
 * This implies that we will have to decide on some kind of arbitrary limit on
 * the number of DmaRequests each process or driver may have.
 **/
class Request
{
public:
	Request(void)
	:
	mapping(NULL)
	{}

	error_t initialize(void)
	{
		error_t ret;

		ret = constraints.initialize(NULL, 0);
		if (ret != ERROR_SUCCESS) { return ret; };

		ret = sGList.initialize();
		if (ret != ERROR_SUCCESS) { return ret; };

		ret = mapping.initialize();
		if (ret != ERROR_SUCCESS) { return ret; };

		return ret;
	}

	~Request(void);

protected:
	Constraints			constraints;
	ScatterGatherList		sGList;
	MappedScatterGatherList		mapping;
};

} /* namespace dma */

} /* namespace fplainn */

#endif
