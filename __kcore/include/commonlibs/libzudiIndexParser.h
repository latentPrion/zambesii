#ifndef _ZUDI_INDEX_H
	#define _ZUDI_INDEX_H

	#include <zui.h>
	#include <arch/paging.h>
	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>
	#include <kernel/common/floodplainn/zui.h>

#define ZUDIIDX		"ZUDI-Idx: "

/**	EXPLANATION:
 * Simple, stateful driver-index manager. Does not use locking. Assumes only one
 * caller will enter it at any given time.
 **/

class Floodplainn;

class ZudiIndexParser
{
friend class Floodplainn;
public:	enum sourceE {
	SOURCE_KERNEL=fplainn::Zui::INDEX_KERNEL,
	SOURCE_RAMDISK=fplainn::Zui::INDEX_RAMDISK,
	SOURCE_EXTERNAL=fplainn::Zui::INDEX_EXTERNAL };

public:
	ZudiIndexParser(sourceE source)
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
	~ZudiIndexParser(void) {}

public:
	error_t getHeader(zui::sHeader *ret)
		{ return driverIndex.read(ret, 0, sizeof(*ret)); }

	/* Finds a driver by the combination of its basepath+shortname
	 * concatenated (i.e, its fullname). If we were to search by shortname
	 * only, then there would be no way to distinguish between potentially
	 * different drivers with the same shortname which exist in multiple
	 * ZUDI index DBs.
	 */
	error_t findDriver(
		zui::sHeader *hdr, utf8Char *fullName,
		zui::driver::sHeader *ret);

	/* Finds a metalanguage by its name. Not the "shortname" of the
	 * metalanguage library, but rather the "provides" line name of the
	 * metalanguage API. If there are multiple candidates that provide a
	 * particular interface, we just return the first result we find.
	 **/
	error_t findMetalanguage(
		zui::sHeader *hdr, utf8Char *metaName,
		zui::driver::sHeader *ret);

	error_t getDriverHeader(
		zui::sHeader *hdr, ubit16 id, zui::driver::sHeader *ret);

	error_t indexedGetDevice(uarch_t index, zui::device::sHeader *mem)
		{ return deviceIndex.indexedRead(mem, index); }

	error_t indexedGetDeviceData(
		zui::device::sHeader *devHeader, ubit16 index,
		zui::device::sAttrData *retobj);

	error_t findDeviceData(
		zui::device::sHeader *devHeader, utf8Char *attrName,
		zui::device::sAttrData *retobj);

	error_t getString(uarch_t offset, utf8Char *retmsg)
		{ return stringIndex.readString(retmsg, offset); }

	error_t getArray(uarch_t offset, uarch_t len, ubit8 *ret)
		{ return stringIndex.read(ret, offset, len); }

	error_t getMessage(
		zui::driver::sHeader *drvHeader, ubit16 index,
		zui::driver::sMessage *ret);

	error_t getMessageString(
		zui::driver::sHeader *drvHeader, ubit16 index,
		utf8Char *ret);

	error_t getMetalanguage(
		zui::driver::sHeader *drvHeader, ubit16 index,
		zui::driver::sMetalanguage *ret);

	error_t indexedGetRank(
		zui::driver::sHeader *metaHeader, uarch_t idx,
		zui::rank::sHeader *retobj);

	error_t indexedGetRankAttrString(
		zui::rank::sHeader *rankHeader, uarch_t idx, utf8Char *retstr);

	error_t indexedGetProvision(
		uarch_t idx, zui::driver::sProvision *retobj)
	{
		return provisionIndex.read(
			retobj, sizeof(*retobj) * idx, sizeof(*retobj));
	}

	error_t getProvisionString(
		zui::driver::sProvision *prov, utf8Char *retstr)
	{
		return stringIndex.readString(retstr, prov->nameOff);
	}

	error_t indexedGetModule(
		zui::driver::sHeader *drvHeader, uarch_t idx,
		zui::driver::sModule *retobj)
	{
		return dataIndex.read(
			retobj,
			drvHeader->modulesOffset + sizeof(*retobj) * idx,
			sizeof(*retobj));
	}

	error_t getModuleString(zui::driver::sModule *module, utf8Char *retstr)
	{
		return stringIndex.readString(retstr, module->fileNameOff);
	}

	error_t indexedGetRegion(
		zui::driver::sHeader *drvHeader, uarch_t idx,
		zui::driver::sRegion *retobj)
	{
		return dataIndex.read(
			retobj,
			drvHeader->regionsOffset + sizeof(*retobj) * idx,
			sizeof(*retobj));
	}

	error_t indexedGetRequirement(
		zui::driver::sHeader *drvHeader, uarch_t idx,
		zui::driver::sRequirement *retobj)
	{
		return dataIndex.read(
			retobj,
			drvHeader->requirementsOffset + sizeof(*retobj) * idx,
			sizeof(*retobj));
	}

	error_t getRequirementString(
		zui::driver::sRequirement *requirement, utf8Char *retstr)
	{
		return stringIndex.readString(retstr, requirement->nameOff);
	}

	error_t indexedGetMetalanguage(
		zui::driver::sHeader *drvHdr, uarch_t idx,
		zui::driver::sMetalanguage *retobj)
	{
		return dataIndex.read(
			retobj,
			drvHdr->metalanguagesOffset + sizeof(*retobj) * idx,
			sizeof(*retobj));
	}

	error_t getMetalanguageString(
		zui::driver::sMetalanguage *metalanguage, utf8Char *retstr)
	{
		return stringIndex.readString(retstr, metalanguage->nameOff);
	}

	error_t indexedGetChildBop(
		zui::driver::sHeader *drvHdr, uarch_t idx,
		zui::driver::sChildBop *retobj)
	{
		return dataIndex.read(
			retobj,
			drvHdr->childBopsOffset + sizeof(*retobj) * idx,
			sizeof(*retobj));
	}

	error_t indexedGetParentBop(
		zui::driver::sHeader *drvHdr, uarch_t idx,
		zui::driver::sParentBop *retobj)
	{
		return dataIndex.read(
			retobj,
			drvHdr->parentBopsOffset + sizeof(*retobj) * idx,
			sizeof(*retobj));
	}

	error_t indexedGetInternalBop(
		zui::driver::sHeader *drvHdr, uarch_t idx,
		zui::driver::sInternalBop *retobj)
	{
		return dataIndex.read(
			retobj,
			drvHdr->internalBopsOffset + sizeof(*retobj) * idx,
			sizeof(*retobj));
	}

private:
	class RandomAccessBuffer
	{
	public:
		RandomAccessBuffer(sourceE source, uarch_t bufferSize)
		:
		fullName(NULL), bufferSize(bufferSize), source(source)
		{
			buffer.rsrc.buffer = buffer.rsrc.bufferEof = NULL;
		}

		error_t initialize(void *source, void *sourceEof);
		error_t initialize(utf8Char *indexPath, utf8Char *fileName);

		~RandomAccessBuffer(void) {}

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
		struct sBufferState
		{
			ubit8		*buffer, *bufferEof;
		};

		SharedResourceGroup<WaitLock, sBufferState>	buffer;
		/* If bufferSize == 0, the class assumes that it is not actually
		 * buffering data from a file-system instance, but rather just
		 * an abstracted layer on top of the in-kernel UDI index.
		 **/
		uarch_t				bufferSize;
		ZudiIndexParser::sourceE	source;
	};

	sourceE			source;
	utf8Char		*indexPath;
	RandomAccessBuffer	driverIndex, dataIndex,
				deviceIndex, rankIndex, provisionIndex,
				stringIndex;
};

#endif

