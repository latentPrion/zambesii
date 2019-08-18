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
	void dump(void)
	{
		printf(NOTICE "ScGthList: %d elements, dumping:\n",
			elements.getNIndexes());

		for (typename SGListElementArray::Iterator it=elements.begin();
			it != elements.end(); ++it)
		{
			__kscgth_element_type_t		tmp = *it;
			paddr_t				p;

			assign_scgth_block_busaddr_to_paddr(
				p, tmp.block_busaddr);

			printf(CC"\tElement: Paddr %P, nBytes %d.\n",
				&p, tmp.block_length);
		}
	}


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
	error_t preallocateEntries(uarch_t nEntries, uarch_t flags=0)
	{
		error_t ret;
		uarch_t fRthi;

		if (nEntries == 0) { return ERROR_SUCCESS; };

		// Only pass on the ResizeableArray flags into resizeToHoldIndex
		fRthi = (FLAG_TEST(flags, PE_FLAGS_UNLOCKED))
			? ResizeableArray<void *>::RTHI_FLAGS_UNLOCKED : 0
		;

		ret = elements.resizeToHoldIndex(
			nEntries - 1, fRthi);

		if (ret != ERROR_SUCCESS) {
			return ret;
		};

		/*	TODO:
		 * Really inefficient? Cycle through and set
		 * the block_length member of each of the new
		 * elements to be 0, indicating that it's an
		 * unused element.
		 */
		return ret;
	}


	enum addFramesE {
		AF_FLAGS_UNLOCKED		= (1<<0)
	};
	status_t addFrames(
		paddr_t p, uarch_t nFrames, sarch_t atIndex, uarch_t flags
		)
	{
		error_t			ret;
		sbit8			dontTakeLock;

		/**	EXPLANATION:
		 * Returns ERROR_SUCCESS if there was no need to allocate a new
		 * SGList element.
		 *
		 * If atIndex is non-negative, it is taken to be a placement index in
		 * the array, at which the frames should be added. Whatever frames exist
		 * at that index will be overwritten.
		 *
		 * Returns 1 if a new element was created.
		 *
		 * Returns negative integer value on error.
		 **/

		dontTakeLock = FLAG_TEST(flags, AF_FLAGS_UNLOCKED) ? 1 : 0;

		if (atIndex >= 0) {
			__kscgth_element_type_t		newElement;

			assign_paddr_to_scgth_block_busaddr(&newElement, p);
			newElement.block_length = PAGING_PAGES_TO_BYTES(nFrames);

			if (!dontTakeLock) { lock(); }
			elements[atIndex] = newElement;
			if (!dontTakeLock) { unlock(); }
			return ERROR_SUCCESS;
		}

		if (!dontTakeLock) { lock(); }

		for (
			typename SGListElementArray::Iterator it=
				elements.begin();
			it != elements.end();
			++it)
		{
			// The dereference is unary operator* on class Iterator.
			__kscgth_element_type_t		*tmp=&*it;

			// Can the new paddr be prepended to this element?
			if (::operator==(p + PAGING_PAGES_TO_BYTES(nFrames), *tmp))
			{
				assign_paddr_to_scgth_block_busaddr(tmp, p);
				tmp->block_length += PAGING_PAGES_TO_BYTES(nFrames);

				if (!dontTakeLock) { unlock(); }
				return ERROR_SUCCESS;
			}
			// Can the new paddr be appended to this element?
			else if (::operator==(p - tmp->block_length, *tmp))
			{
				tmp->block_length += PAGING_PAGES_TO_BYTES(nFrames);

				if (!dontTakeLock) { unlock(); }
				return ERROR_SUCCESS;
			};
		};

		/* If we reached here, then we need to add a new element altogether.
		 **/
		uarch_t				prevNIndexes;
		__kscgth_element_type_t		newElement;

		// Resize the list to hold the new SGList element.
		prevNIndexes = elements.unlocked_getNIndexes();
		ret = elements.resizeToHoldIndex(
			prevNIndexes,
			SGListElementArray::RTHI_FLAGS_UNLOCKED);

		if (!dontTakeLock) { unlock(); }

		if (ret != ERROR_SUCCESS) { return ret; };

		// Initialize the new element's values.
		memset(&newElement, 0, sizeof(newElement));
		assign_paddr_to_scgth_block_busaddr(&newElement, p);
		newElement.block_length = PAGING_PAGES_TO_BYTES(nFrames);

		// Finally add it to the list.
		elements[prevNIndexes] = newElement;
		return 1;
	}

	enum getNFramesE {
		GNF_FLAGS_UNLOCKED		= (1<<0)
	};
	uarch_t getNFrames(uarch_t flags=0)
	{
		uarch_t							ret=0;
		typename SGListElementArray::Iterator	it;

		if (!FLAG_TEST(flags, ScatterGatherList::GNF_FLAGS_UNLOCKED))
			{ lock(); }

		for (it = elements.begin(); it != elements.end(); ++it)
		{
			__kscgth_element_type_t		*el = &*it;
			uarch_t				currNFrames;

			currNFrames = PAGING_BYTES_TO_PAGES(el->block_length);
			ret += currNFrames;
		}

		if (!FLAG_TEST(flags, ScatterGatherList::GNF_FLAGS_UNLOCKED))
			{ unlock(); }

		return ret;
	}

	status_t getElements(
		void *outarr, uarch_t nelem, ubit8 *outarrType
		)
	{
		class SGListElementArray::Iterator	it;
		uarch_t							i;

		if (outarr == NULL || outarrType == NULL) { return ERROR_INVALID_ARG; }

		if ((*outarrType & (UDI_SCGTH_32 | UDI_SCGTH_64)) == 0) {
			printf(ERROR SGLIST"getElements: Caller must state supported "
				"output list formats. Pass UDI_SCGTH_[32|64].\n");

			return ERROR_INVALID_ARG_VAL;
		}

		/* Prefer to support 64-bit output lists if the caller indicates support
		 * for them.
		 *
		 * But only one sglist format flag must be set by the kernel as the
		 * output `outarrType` returned to the user.
		 */
		if (FLAG_TEST(*outarrType, UDI_SCGTH_64)) {
			*outarrType = UDI_SCGTH_64;
		} else {
			// Nothing to do -- 32 was already set by caller.
		}

		lock();

		// See if the supplied outarr can hold all the elements.
		for (i=0, it=elements.begin();
			i < nelem && it != elements.end();
			++it, i++)
		{
			__kscgth_element_type_t		tmp = *it;
			paddr_t				p;

			assign_scgth_block_busaddr_to_paddr(p, tmp.block_busaddr);

			switch (*outarrType)
			{
			case UDI_SCGTH_32:
				{
				udi_scgth_element_32_t			*outarr32;

				outarr32 = static_cast<udi_scgth_element_32_t *>(outarr);

				outarr32[i].block_length = tmp.block_length;
				assign_paddr_to_scgth_block_busaddr(&outarr32[i], p);
				}
				break;
			default:
				{
				udi_scgth_element_64_t			*outarr64;

				outarr64 = static_cast<udi_scgth_element_64_t *>(outarr);

				outarr64[i].block_length = tmp.block_length;
				assign_paddr_to_scgth_block_busaddr(&outarr64[i], p);
				}
			}
		}

		unlock();
		return i;
	}

	error_t resize(uarch_t nFrames)
	{
		uarch_t		currNFrames;
		status_t	nNewFrames;

		/**	EXPLANATION:
		 * Causes this SGList's physical memory to be expanded to at least
		 * nFrames.
		 **/
		if (nFrames == 0) { return ERROR_SUCCESS; }

		lock();
		currNFrames = getNFrames(GNF_FLAGS_UNLOCKED);
		nNewFrames = unlocked_resize(nFrames);
		unlock();

		if (currNFrames + nNewFrames < nFrames)
		{
			// Just to be sure about the behaviour of constrainedGetFrames.
			assert_fatal(nNewFrames < 0);

			printf(ERROR SGLIST"resize(nFrames): Op failed. "
				"PrevNFrames %d, %d allocated newly and will be "
				"freed.\n",
				nFrames, currNFrames, nNewFrames);

			return nNewFrames;
		}

		return ERROR_SUCCESS;
	}

	/* Returns the number of elements that were discarded
	 * due to compaction.
	 **/
	uarch_t compact(void);

	// Maps this SGList and records the page list using its MappedSGList.
	error_t map(void)
	{
		uarch_t			nFrames, nFramesMapped;
		Thread			*currThread;
		typename SGListElementArray::Iterator		it;
		void			*vaddr;
		uintptr_t		currVaddr;
		error_t			ret;
		status_t		status;
		paddr_t			p;

		/*	EXPLANATION:
		 * Map a scgth list into the vaddrspace of the caller. This is fine
		 * because only the caller should have a handle-ID to any sglist.
		 *
		 * MappedSGLists are not demand mapped. They are fully mapped and
		 * committed.
		 */

		currThread = cpuTrib.getCurrentCpuStream()->taskStream
			.getCurrentThread();

		lock();

		nFrames = getNFrames(GNF_FLAGS_UNLOCKED);

		vaddr = scatterGatherLists::getPages(currThread, nFrames);
		if (vaddr == NULL)
		{
			unlock();
			printf(ERROR SGLIST"map: Failed to alloc vmem to map the %d "
				"frames in the list.\n",
				nFrames);

			return ERROR_MEMORY_NOMEM_VIRTUAL;
		}

		currVaddr = (uintptr_t)vaddr;
		nFramesMapped = 0;
		for (it = elements.begin(); it != elements.end(); ++it)
		{
			__kscgth_element_type_t			*tmp=&*it;
			uarch_t					nPagesInBlock;

			// Map each paddr + length segment.
			assign_scgth_block_busaddr_to_paddr(p, tmp->block_busaddr);
			nPagesInBlock = PAGING_BYTES_TO_PAGES(tmp->block_length);

			status = scatterGatherLists::wprMapInc(
				currThread, (void *)currVaddr, p, nPagesInBlock);

			if (status < (sarch_t)nPagesInBlock)
			{
				unlock();

				printf(ERROR SGLIST"map: Failed to map all %d pages "
					"for SGList element P%P, %d frames.\n",
					nPagesInBlock, &p);

				ret = ERROR_MEMORY_VIRTUAL_PAGEMAP;
				goto unmapAndReleaseVmem;
			}

			currVaddr += tmp->block_length;
			assert_fatal((currVaddr & PAGING_BASE_MASK_LOW) == 0);
			nFramesMapped += nPagesInBlock;

			// Ensure we don't exceed the amount of loose vmem allocated.
			assert_fatal(nFramesMapped <= nFrames);
		}

		unlock();

		ret = mapping.trackPages(vaddr, nFrames);
		if (ret != ERROR_SUCCESS)
		{
			printf(ERROR SGLIST"map: Failed to track vmem pages at %p in "
				"SGList mapping.\n",
				vaddr);

			// 'ret' was already set.
			goto unmapAndReleaseVmem;
		}

		return ERROR_SUCCESS;

	unmapAndReleaseVmem:
		status = scatterGatherLists::wprUnmap(currThread, vaddr, nFrames);
		if (status < (sarch_t)nFrames)
		{
			printf(FATAL SGLIST"map: Failed to clean up. Only %d of %d "
				"pages were unmapped from vaddrspace.\n",
				status, nFrames);

			// Fallthrough.
		}

		scatterGatherLists::releasePages(currThread, vaddr, nFrames);
		return ret;
	}


	// Remaps this SGList.
	error_t remap(void)
	{
		unmap();
		return map();
	}

	// Destroys a mapping to this SGList.
	void unmap(void);

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
