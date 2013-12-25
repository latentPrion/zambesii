#ifndef _ZUDI_INDEX_H
	#define _ZUDI_INDEX_H

	#include <zudiIndex.h>
	#include <arch/paging.h>
	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>
	#include <kernel/common/zudiIndexServer.h>

#define ZUDIIDX		"ZUDI-Idx: "

/**	EXPLANATION:
 * Simple, stateful driver-index manager. Does not use locking. Assumes only one
 * caller will enter it at any given time.
 **/

class floodplainnC;

class zudiIndexParserC
{
friend class floodplainnC;
public:	enum sourceE {
	SOURCE_KERNEL=zudiIndexServer::INDEX_KERNEL,
	SOURCE_RAMDISK=zudiIndexServer::INDEX_RAMDISK,
	SOURCE_EXTERNAL=zudiIndexServer::INDEX_EXTERNAL };

public:
	zudiIndexParserC(sourceE source)
	:
	source(source), indexPath(NULL),
	driverIndex(source, PAGING_BASE_SIZE),
	dataIndex(source, PAGING_BASE_SIZE),
	deviceIndex(source, PAGING_BASE_SIZE),
	rankIndex(source, PAGING_BASE_SIZE),
	provisionIndex(source, PAGING_BASE_SIZE),
	stringIndex(source, PAGING_BASE_SIZE)
	{}

	error_t initialize(utf8Char *indexPath);
	~zudiIndexParserC(void) {}

public:
	error_t getHeader(zudi::headerS *ret)
		{ return driverIndex.read(ret, 0, sizeof(*ret)); }

	/* Finds a driver by the combination of its basepath+shortname
	 * concatenated (i.e, its fullname). If we were to search by shortname
	 * only, then there would be no way to distinguish between potentially
	 * different drivers with the same shortname which exist in multiple
	 * ZUDI index DBs.
	 */
	error_t findDriver(
		zudi::headerS *hdr, utf8Char *fullName,
		zudi::driver::headerS *ret);

	/* Finds a metalanguage by its name. Not the "shortname" of the
	 * metalanguage library, but rather the "provides" line name of the
	 * metalanguage API. If there are multiple candidates that provide a
	 * particular interface, we just return the first result we find.
	 **/
	error_t findMetalanguage(
		zudi::headerS *hdr, utf8Char *metaName,
		zudi::driver::headerS *ret);

	error_t getDriverHeader(
		zudi::headerS *hdr, ubit16 id, zudi::driver::headerS *ret);

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

	error_t indexedGetModule(
		zudi::driver::headerS *drvHeader, uarch_t idx,
		zudi::driver::moduleS *retobj)
	{
		return dataIndex.read(
			retobj,
			drvHeader->modulesOffset + sizeof(*retobj) * idx,
			sizeof(*retobj));
	}

	error_t getModuleString(zudi::driver::moduleS *module, utf8Char *retstr)
	{
		return stringIndex.readString(retstr, module->fileNameOff);
	}

	error_t indexedGetRegion(
		zudi::driver::headerS *drvHeader, uarch_t idx,
		zudi::driver::regionS *retobj)
	{
		return dataIndex.read(
			retobj,
			drvHeader->regionsOffset + sizeof(*retobj) * idx,
			sizeof(*retobj));
	}

	error_t indexedGetRequirement(
		zudi::driver::headerS *drvHeader, uarch_t idx,
		zudi::driver::requirementS *retobj)
	{
		return dataIndex.read(
			retobj,
			drvHeader->requirementsOffset + sizeof(*retobj) * idx,
			sizeof(*retobj));
	}

	error_t getRequirementString(
		zudi::driver::requirementS *requirement, utf8Char *retstr)
	{
		return stringIndex.readString(retstr, requirement->nameOff);
	}

	error_t indexedGetMetalanguage(
		zudi::driver::headerS *drvHdr, uarch_t idx,
		zudi::driver::metalanguageS *retobj)
	{
		return dataIndex.read(
			retobj,
			drvHdr->metalanguagesOffset + sizeof(*retobj) * idx,
			sizeof(*retobj));
	}

	error_t getMetalanguageString(
		zudi::driver::metalanguageS *metalanguage, utf8Char *retstr)
	{
		return stringIndex.readString(retstr, metalanguage->nameOff);
	}

	error_t indexedGetChildBop(
		zudi::driver::headerS *drvHdr, uarch_t idx,
		zudi::driver::childBopS *retobj)
	{
		return dataIndex.read(
			retobj,
			drvHdr->childBopsOffset + sizeof(*retobj) * idx,
			sizeof(*retobj));
	}

	error_t indexedGetParentBop(
		zudi::driver::headerS *drvHdr, uarch_t idx,
		zudi::driver::parentBopS *retobj)
	{
		return dataIndex.read(
			retobj,
			drvHdr->parentBopsOffset + sizeof(*retobj) * idx,
			sizeof(*retobj));
	}

	error_t indexedGetInternalBop(
		zudi::driver::headerS *drvHdr, uarch_t idx,
		zudi::driver::internalBopS *retobj)
	{
		return dataIndex.read(
			retobj,
			drvHdr->internalBopsOffset + sizeof(*retobj) * idx,
			sizeof(*retobj));
	}

private:
	class randomAccessBufferC
	{
	public:
		randomAccessBufferC(sourceE source, uarch_t bufferSize)
		:
		fullName(NULL), bufferSize(bufferSize), source(source)
		{
			buffer.rsrc.buffer = buffer.rsrc.bufferEnd = NULL;
		}

		error_t initialize(void *source, void *sourceEnd);
		error_t initialize(utf8Char *indexPath, utf8Char *fileName);

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
		uarch_t				bufferSize;
		zudiIndexParserC::sourceE	source;
	};

	sourceE			source;
	utf8Char		*indexPath;
	randomAccessBufferC	driverIndex, dataIndex,
				deviceIndex, rankIndex, provisionIndex,
				stringIndex;
};

#endif

