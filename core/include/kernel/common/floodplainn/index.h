#ifndef _ZUDI_INDEX_H
	#define _ZUDI_INDEX_H

	#include <zudiIndex.h>
	#include <arch/paging.h>
	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>

#define ZUDIIDX		"ZUDI-Idx: "

/**	EXPLANATION:
 * Simple, stateful driver-index manager. Does not use locking. Assumes only one
 * caller will enter it at any given time.
 **/

class floodplainnC;

class zudiIndexC
{
friend class floodplainnC;
public:	enum sourceE { SOURCE_KERNEL=0, SOURCE_RAMDISK, SOURCE_EXTERNAL };
private:
	// This constructor is only for use by floodplainnC.
	zudiIndexC(void)
	:
	source(SOURCE_KERNEL), indexPath(NULL),
	driverIndex(PAGING_BASE_SIZE), dataIndex(PAGING_BASE_SIZE),
	deviceIndex(PAGING_BASE_SIZE), rankIndex(PAGING_BASE_SIZE),
	provisionIndex(PAGING_BASE_SIZE), stringIndex(PAGING_BASE_SIZE)
	{}

public:
	zudiIndexC(sourceE source)
	:
	source(source), indexPath(NULL),
	driverIndex(PAGING_BASE_SIZE), dataIndex(PAGING_BASE_SIZE),
	deviceIndex(PAGING_BASE_SIZE), rankIndex(PAGING_BASE_SIZE),
	provisionIndex(PAGING_BASE_SIZE), stringIndex(PAGING_BASE_SIZE)
	{}

	error_t initialize(utf8Char *indexPath);
	~zudiIndexC(void) {}

public:
	error_t getHeader(zudi::headerS *ret)
		{ return driverIndex.read(ret, 0, sizeof(*ret)); }

	error_t findDriver(utf8Char *fullName, zudi::driver::headerS *ret);
	error_t findMetalanguage(utf8Char *metaName, zudi::driver::headerS *ret);

	error_t getDriverHeader(ubit16 id, zudi::driver::headerS *ret);

	error_t indexedGetDevice(uarch_t index, zudi::device::headerS *mem)
		{ return deviceIndex.indexedRead(mem, index); }

	error_t indexedGetDeviceData(
		zudi::device::headerS *devHeader, ubit16 index,
		zudi::device::attrDataS *retobj);

	error_t findDeviceData(
		zudi::device::headerS *devHeader, utf8Char *attrName,
		zudi::device::attrDataS *retobj);

	error_t getString(uarch_t offset, utf8Char *retmsg)
		{ return stringIndex.readString(retmsg, offset); }

	error_t getArray(uarch_t offset, uarch_t len, ubit8 *ret)
		{ return stringIndex.read(ret, offset, len); }

	error_t getMessage(
		zudi::driver::headerS *drvHeader, ubit16 index,
		zudi::driver::messageS *ret);

	error_t getMessageString(
		zudi::driver::headerS *drvHeader, ubit16 index,
		utf8Char *ret);

	error_t getMetalanguage(
		zudi::driver::headerS *drvHeader, ubit16 index,
		zudi::driver::metalanguageS *ret);

	error_t indexedGetRank(
		zudi::driver::headerS *metaHeader, uarch_t idx,
		zudi::rank::headerS *retobj);

	error_t indexedGetRankAttrString(
		zudi::rank::headerS *rankHeader, uarch_t idx, utf8Char *retstr);


private:
	class randomAccessBufferC
	{
	public:
		randomAccessBufferC(uarch_t bufferSize)
		:
		fullName(NULL), bufferSize(bufferSize)
		{
			buffer.rsrc.buffer = buffer.rsrc.bufferEnd = NULL;
		}

		error_t initialize(
			utf8Char *indexPath=NULL, utf8Char *fileName=NULL);

		error_t initialize(void *source, void *sourceEnd)
		{
			bufferSize = 0;
			buffer.rsrc.buffer = (ubit8 *)source;
			buffer.rsrc.bufferEnd = (ubit8 *)sourceEnd;
			return ERROR_SUCCESS;
		}

		~randomAccessBufferC(void) {}

	public:
		error_t readString(utf8Char *stringbuff, uarch_t offset);
		error_t read(void *buff, uarch_t offset, uarch_t nBytes);
		template <class T> error_t indexedRead(T *mem, uarch_t index)
		{
			return read(mem, index * sizeof(*mem), sizeof(*mem));
		}

		void discardBuffer(void);

	private:
		// Name of the file this object is buffering.
		utf8Char	*fullName;
		// Pointers to the buffer in memory and its end.
		struct bufferStateS
		{
			ubit8		*buffer, *bufferEnd;
		};

		sharedResourceGroupC<waitLockC, bufferStateS>	buffer;
		/* If bufferSize == 0, the class assumes that it is not actually
		 * buffering data from a file-system instance, but rather just
		 * an abstracted layer on top of the in-kernel UDI index.
		 **/
		uarch_t		bufferSize;
	};

	sourceE			source;
	utf8Char		*indexPath;
	randomAccessBufferC	driverIndex, dataIndex,
				deviceIndex, rankIndex, provisionIndex,
				stringIndex;
};

#endif

