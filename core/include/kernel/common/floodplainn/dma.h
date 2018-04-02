#ifndef _ZBZ_DMA_H
	#define _ZBZ_DMA_H

	#define UDI_VERSION 0x101
	#include <udi.h>
	#undef UDI_VERSION
	#include <__kclasses/resizeableArray.h>
	#include <__kclasses/list.h>

namespace fplainn
{

namespace dma
{

/**	EXPLANATION:
 * This class encapsulates the kernel's ability to read/write from/to a DMA
 * SGlist. SGLists are in physical memory, so the kernel must map them before it
 * can alter their contents.
 *
 * This class provides methods such as memset(), memcpy(), etc.
 ******************************************************************************/
class ScatterGatherList;

class MappedScatterGatherList
/*: public SparseBuffer*/
{
protected:
	struct sListNode
	{
		List<sListNode>::sHeader	listHeader;
		ubit8				*vaddr;
		uarch_t				nBytes;
	};

public:
	MappedScatterGatherList(ScatterGatherList *_parent)
	:
	parent(_parent)
	{}

	error_t initialize()
	{
		error_t		ret;

		ret = pageArray.initialize();
		return ret;
	}

	~MappedScatterGatherList(void) {}

public:
	virtual void memset8(
		uarch_t offset, ubit8 value, uarch_t nBytes);
	virtual void memset16(
		uarch_t offset, ubit8 value, uarch_t nBytes);
	virtual void memset32(
		uarch_t offset, ubit8 value, uarch_t nBytes);

	virtual void memcpy(
		uarch_t offset, void *mem, uarch_t nBytes);
	virtual void memcpy(
		void *mem, uarch_t offset, uarch_t nBytes);

	void memmove(
		uarch_t destOff, uarch_t srcOff,
		uarch_t nBytes);

	error_t addPages(void *vaddr, uarch_t nBytes);
	error_t removePages(void *vaddr, uarch_t nBytes);
	void compact(void);

protected:
	ResizeableArray<void *>	pageArray;
	ScatterGatherList       *parent;
	void			*vaddr;
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
public:
	enum eAddressSize {
		ADDR_SIZE_UNKNOWN, ADDR_SIZE_32, ADDR_SIZE_64
	};

	ScatterGatherList(void)
	:
	addressSize(ADDR_SIZE_UNKNOWN)
	{
		memset(&udiScgthList, 0, sizeof(udiScgthList));
	}

	error_t initialize(eAddressSize addrSize)
	{
		error_t		ret;

		addressSize = addrSize;
		ret = elements32.initialize();
		if (ret != ERROR_SUCCESS) { return ret; };

		ret = elements64.initialize();
		if (ret != ERROR_SUCCESS) { return ret; };

		return ERROR_SUCCESS;
	}

	~ScatterGatherList(void) {}

public:
	void dump(void)
	{
		if (addressSize == ADDR_SIZE_32) {
			dump(&elements32);
		} else {
			dump(&elements64);
		};
	}

	error_t preallocateEntries(uarch_t nEntries)
	{
		if (nEntries == 0) { return ERROR_SUCCESS; };
		if (addressSize == ADDR_SIZE_UNKNOWN) {
			return ERROR_UNINITIALIZED;
		};

		if (addressSize == ADDR_SIZE_32)
		{
			return preallocateEntries(
				&elements32, nEntries);
		}
		else
		{
			return preallocateEntries(
				&elements64, nEntries);
		};
	}

	status_t addFrames(
		paddr_t p, uarch_t nFrames, sarch_t atIndex=-1)
	{
		status_t		ret;

		/**	EXPLANATION:
		 * Iterates through the list first to see if it
		 * can add the new frames to an existing
		 * element.
		 **/
		if (addressSize == ADDR_SIZE_UNKNOWN) {
			return ERROR_UNINITIALIZED;
		};

		if (addressSize == ADDR_SIZE_32)
		{
			ret = addFrames(
				&elements32, p, nFrames,
				atIndex);
		}
		else
		{
			ret = addFrames(
				&elements64, p, nFrames,
				atIndex);
		};

		if (ret > ERROR_SUCCESS)
		{
			// TODO: Should be atomic_add.
			udiScgthList.scgth_num_elements++;
		};

		return ret;
	}

	/* Returns the number of elements that were discarded
	 * due to compaction.
	 **/
	uarch_t compact(void);

	// Returns a virtual mapping to this SGList.
	error_t map(MappedScatterGatherList *retMapping)
	{
		if (addressSize == ADDR_SIZE_32) {
			return map(&elements32, retMapping);
		} else {
			return map(&elements64, retMapping);
		}
	}

	// Returns a re-done mapping of this SGList.
	error_t remap(
		MappedScatterGatherList *oldMapping,
		MappedScatterGatherList *newMapping);

	// Destroys a mapping to this SGList.
	void unmap(MappedScatterGatherList *mapping);

private:
	template <class scgth_elements_type>
	void dump(ResizeableArray<scgth_elements_type> *list);

	template <class T>
	error_t preallocateEntries(T *array, uarch_t nEntries)
	{
		error_t ret;

		ret = array->resizeToHoldIndex(
			nEntries - 1);

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

	template <class scgth_elements_type>
	error_t addFrames(
		ResizeableArray<scgth_elements_type> *list,
		paddr_t p, uarch_t nFrames, sarch_t atIndex=-1);

	template <class scgth_elements_type>
	error_t map(
		ResizeableArray<scgth_elements_type> *list,
		MappedScatterGatherList *retMapping);

public:
	typedef ResizeableArray<udi_scgth_element_32_t>
					SGList32;
	typedef ResizeableArray<udi_scgth_element_64_t>
					SGList64;

	eAddressSize			addressSize;
	SGList32			elements32;
	SGList64			elements64;
	udi_scgth_t			udiScgthList;
};

/**	EXPLANATION:
 * This class represents Zambesii's UDI DMA constraints feature.
 */
class Constraints;
namespace constraints
{

class Compiler
{
public:
	Compiler(Constraints *_parent)
	:
	parent(_parent)
	{
		memset(&i, 0, sizeof(i));
	}

	error_t compile(void);
	void dump(void);

public:
	struct {
		sbit8		partialAllocationsDisallowed,
				sequentialAccessHint;
		ubit8		addressableBits,
				fixedBits;
		uarch_t		pfnSkipStride;
		paddr_t		startPfn, beyondEndPfn,
				minElementGranularityNFrames,
				maxNContiguousFrames,
				slopInBits, slopOutBits,
				slopOutExtra, slopBarrierBits,
				fixedBitsValue;
		ScatterGatherList::eAddressSize addressSize;
	} i;

private:
	Constraints	*parent;
};

}

class Constraints
{
public:
	friend class MemoryBmp;

	typedef ResizeableArray<udi_dma_constraints_attr_spec_t>
		AttrArray;

	Constraints(void)
	:
	compiler(this)
	{}

	error_t initialize(void)
	{
		error_t		ret;

		ret = attrs.initialize();
		if (ret != ERROR_SUCCESS) { return ret; };

		return ret;
	}

public:
	void dump(void);
	static utf8Char *getAttrTypeName(
		udi_dma_constraints_attr_t a);

	sbit8 attrAlreadySet(udi_dma_constraints_attr_t a)
	{
		attrs.lock();

		for (
			AttrArray::Iterator it=attrs.begin();
			it != attrs.end();
			++it)
		{
			udi_dma_constraints_attr_spec_t *s =
				&(*it);

			if (s->attr_type == a)
			{
				attrs.unlock();
				return 1;
			};
		};

		attrs.unlock();
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
	constraints::Compiler		compiler;
	#define N_ATTR_TYPE_NAMES	(32)
	static utf8Char			*attrTypeNames[N_ATTR_TYPE_NAMES];
	AttrArray			attrs;
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

	error_t initialize(enum ScatterGatherList::eAddressSize addrSize)
	{
		error_t ret;

		ret = constraints.initialize();
		if (ret != ERROR_SUCCESS) { return ret; };

		ret = sGList.initialize(addrSize);
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

/**	Template definitions:
 * These inline template operations make it easier to code assignments to
 * UDI's scatter-gather bignum types.
 ******************************************************************************/

inline int operator ==(paddr_t p, udi_scgth_element_32_t s)
{
	return (p == s.block_busaddr);
}

inline int operator ==(paddr_t p, udi_scgth_element_64_t s)
{
#if __VADDR_NBITS__ == 32 && !defined(CONFIG_ARCH_x86_32_PAE)
	struct s64BitInt
	{
		ubit32		low, high;
	} *e = (s64BitInt *)&s.block_busaddr;

	if (e->high != 0) { return 0; };
	return p == e->low;
#else
	paddr_t		*p2 = (paddr_t *)&s.block_busaddr;

	return p == *p2;
#endif
}

inline void assign_paddr_to_scgth_block_busaddr(udi_scgth_element_32_t *u32, paddr_t p)
{
#if __VADDR_NBITS__ == 32 && !defined(CONFIG_ARCH_x86_32_PAE)
	// No-pae-32bit-paddr being assigned to a 32-bit-block_busaddr.
	u32->block_busaddr = p.getLow();
#else
	/* Pae-64bit-paddr being assigned to a 32bit-block_busaddr.
	 *
	 * Requires us to cut off the high bits of the pae-64bit-paddr.
	 *
	 * In such a case, we would have clearly chosen to build a 32-bit
	 * SGList, so we should not be trying to add any frames with the high
	 * bits set.
	 **/
	if ((p >> 32).getLow() != 0)
	{
		panic("Trying to add a 64-bit paddr with high bits set, to a "
			"32-bit SGList.");
	};

	u32->block_busaddr = p.getLow();
#endif
}

inline void assign_paddr_to_scgth_block_busaddr(udi_scgth_element_64_t *u64, paddr_t p)
{
#if __VADDR_NBITS__ == 32 && !defined(CONFIG_ARCH_x86_32_PAE)
	/* No-pae-32bit-paddr being assigned to a 64-bit-block_busaddr.
	 *
	 * Just requires us to assign the paddr to the low 32 bits of the
	 * block_busaddr. We also clear the high bits to be pedantic.
	 **/
	struct s64BitInt
	{
		ubit32		low, high;
	} *e = (s64BitInt *)&u64->block_busaddr;

	e->high = 0;
	e->low = p.getLow();
#else
	// Pae-64bit-paddr being assigned to a 64-bit-block_busaddr.
	paddr_t		*p2 = (paddr_t *)&u64->block_busaddr;

	*p2 = p;
#endif
}

inline void assign_scgth_block_busaddr_to_paddr(paddr_t &p, udi_busaddr64_t u64)
{
#if __VADDR_NBITS__ == 32 && !defined(CONFIG_ARCH_x86_32_PAE)
	struct s64BitInt
	{
		ubit32		low, high;
	} *s = (s64BitInt *)&u64;

	if (s->high != 0) {
		panic(CC"High bits set in udi_busaddr64_t on non-PAE build.\n");
	};

	p = s->low;
#else
	paddr_t			*p2 = (paddr_t *)&u64;

	p = *p2;
#endif
}

inline void assign_scgth_block_busaddr_to_paddr(paddr_t &p, udi_ubit32_t u32)
{
	p = u32;
}


/** Inline methods:
 ******************************************************************************/

template <class scgth_elements_type>
void fplainn::dma::ScatterGatherList::dump(
	ResizeableArray<scgth_elements_type> *list
	)
{
	printf(NOTICE "ScGthList: %d elements, dumping:\n", list->getNIndexes());
	for (typename ResizeableArray<scgth_elements_type>::Iterator it=list->begin();
		it != list->end(); ++it)
	{
		scgth_elements_type	tmp = *it;
		paddr_t				p;

		assign_scgth_block_busaddr_to_paddr(p, tmp.block_busaddr);
		printf(CC"\tElement: Paddr %P, nBytes %d.\n",
			&p, tmp.block_length);
	}
}

template <class scgth_elements_type>
error_t fplainn::dma::ScatterGatherList::addFrames(
	ResizeableArray<scgth_elements_type> *list, paddr_t p, uarch_t nFrames,
	sarch_t atIndex
	)
{
	error_t			ret;

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

	if (atIndex >= 0) {
		scgth_elements_type	newElement;

		assign_paddr_to_scgth_block_busaddr(&newElement, p);
		newElement.block_length = PAGING_PAGES_TO_BYTES(nFrames);

		list->lock();
		(*list)[atIndex] = newElement;
		list->unlock();
		return ERROR_SUCCESS;
	}

	list->lock();

	for (
		typename ResizeableArray<scgth_elements_type>::Iterator it=
			list->begin();
		it != list->end();
		++it)
	{
		// The dereference is unary operator* on class Iterator.
		scgth_elements_type		*tmp=&*it;

		// Can the new paddr be prepended to this element?
		if (::operator==(p + PAGING_PAGES_TO_BYTES(nFrames), *tmp))
		{
			assign_paddr_to_scgth_block_busaddr(tmp, p);
			tmp->block_length += PAGING_PAGES_TO_BYTES(nFrames);

			list->unlock();
			return ERROR_SUCCESS;
		}
		// Can the new paddr be appended to this element?
		else if (::operator==(p - tmp->block_length, *tmp))
		{
			tmp->block_length += PAGING_PAGES_TO_BYTES(nFrames);

			list->unlock();
			return ERROR_SUCCESS;
		};
	};

	/* If we reached here, then we need to add a new element altogether.
	 **/
	uarch_t			prevNIndexes;
	scgth_elements_type	newElement;

	// Resize the list to hold the new SGList element.
	prevNIndexes = list->unlocked_getNIndexes();
	ret = list->resizeToHoldIndex(
		prevNIndexes,
		ResizeableArray<scgth_elements_type>::RTHI_FLAGS_UNLOCKED);

	list->unlock();

	if (ret != ERROR_SUCCESS) { return ret; };

	// Initialize the new element's values.
	memset(&newElement, 0, sizeof(newElement));
	assign_paddr_to_scgth_block_busaddr(&newElement, p);
	newElement.block_length = PAGING_PAGES_TO_BYTES(nFrames);

	// Finally add it to the list.
	(*list)[prevNIndexes] = newElement;
	return 1;
}

#endif