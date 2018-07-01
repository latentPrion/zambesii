#ifndef _Z_UDI_INDEX_H
	#define _Z_UDI_INDEX_H

	#define UDI_VERSION	0x101
	#include <udi.h>
	#undef UDI_VERSION
	#include <stdint.h>

/**	EXPLANATION:
 * This header is contained in all UDI index files. The kernel uses it to
 * quickly gain information about the whole of the index at a glance. Versioning
 * of the struct layouts used is also included for forward expansion.
 **/

#define ZUI_MESSAGE_MAXLEN		(150)
#define ZUI_FILENAME_MAXLEN		(64)

namespace zui
{
	struct sHeader
	{
		// Version of the record format used in this index file.
		// "endianness" is a NULL-terminated string of either "le" or "be".
		char		endianness[4];
		uint16_t	majorVersion, minorVersion;
		uint32_t	nRecords, nextDriverId;
		uint32_t	nSupportedDevices, nSupportedMetas;
		uint8_t		reserved[60];
	};

	namespace device
	{
		struct sHeader
		{
			uint32_t	driverId;
			uint16_t	index;
			uint16_t	messageIndex, metaIndex;
			uint8_t		nAttributes;
			uint32_t	dataOff;
		};

		struct sAttrData
		{
			uint8_t		attr_type, attr_length;
			uint32_t	attr_nameOff, attr_valueOff;
		};

		struct _sAttrData
		:
		public udi_instance_attr_list_t
		{
#if !defined(__ZAMBESII_KERNEL_SOURCE__)
			int writeOut(FILE *outfile, FILE *stringfile);
#endif
		};

		#define ZUI_DEVICE_MAX_NATTRS		(20)
		struct _deviceS
		{
#if !defined(__ZAMBESII_KERNEL_SOURCE__)
			int  writeOut(
				FILE *headerfile, FILE *datafile,
				FILE *strings);
#endif

			struct sHeader		h;
			struct _sAttrData	d[ZUI_DEVICE_MAX_NATTRS];
		};
	}

	#define ZUI_DRIVER_SHORTNAME_MAXLEN		(16)
	#define ZUI_DRIVER_RELEASE_MAXLEN		(32)
	#define ZUI_DRIVER_BASEPATH_MAXLEN		(128)
	namespace driver
	{
		enum typeE	{ DRIVERTYPE_DRIVER, DRIVERTYPE_METALANGUAGE };

		struct sHeader
		{
			// TODO: Add support for custom attributes.
			uint32_t	id, type;
			uint16_t	nameIndex, supplierIndex, contactIndex,
			// Category index is only valid for meta libs, not drivers.
					categoryIndex;
			char		shortName[ZUI_DRIVER_SHORTNAME_MAXLEN];
			char		releaseString[ZUI_DRIVER_RELEASE_MAXLEN];
			char		releaseStringIndex;
			uint32_t	requiredUdiVersion;
			char		basePath[ZUI_DRIVER_BASEPATH_MAXLEN];

			/* dataFileOffset is the offset within data.zudi-index.
			 * rankFileOffset is the offset within ranks.zudi-index.
			 **/
			uint32_t	dataFileOffset, rankFileOffset,
					deviceFileOffset, provisionFileOffset;
			uint8_t		nMetalanguages, nChildBops, nParentBops,
					nInternalBops,
					nModules, nRequirements,
					nMessages, nDisasterMessages,
					nMessageFiles, nReadableFiles, nRegions,
					nDevices, nRanks, nProvisions;

			uint32_t	requirementsOffset, metalanguagesOffset,
					childBopsOffset, parentBopsOffset,
					internalBopsOffset, modulesOffset,
					regionsOffset, messagesOffset,
					disasterMessagesOffset,
					messageFilesOffset, readableFilesOffset;
		};

		#define ZUI_DRIVER_MAX_NREQUIREMENTS		(16)
		#define ZUI_DRIVER_MAX_NMETALANGUAGES		(16)
		#define ZUI_DRIVER_MAX_NCHILD_BOPS		(12)
		#define ZUI_DRIVER_MAX_NPARENT_BOPS		(8)
		#define ZUI_DRIVER_MAX_NINTERNAL_BOPS		(24)
		#define ZUI_DRIVER_MAX_NMODULES		(16)

		#define ZUI_DRIVER_METALANGUAGE_MAXLEN		(32)
		#define ZUI_DRIVER_REQUIREMENT_MAXLEN		\
					(ZUI_DRIVER_METALANGUAGE_MAXLEN)

		struct sRequirement
		{
			uint32_t	version;
			uint32_t	nameOff;
		};

		struct _sRequirement
		{
#if !defined(__ZAMBESII_KERNEL_SOURCE__)
			int writeOut(FILE *dataF, FILE *stringF);
#endif

			uint32_t	version;
			char		name[ZUI_DRIVER_REQUIREMENT_MAXLEN];
		};

		struct sMetalanguage
		{
			uint16_t	index;
			uint32_t	nameOff;
		};

		struct _sMetalanguage
		{
#if !defined(__ZAMBESII_KERNEL_SOURCE__)
			int writeOut(FILE *dataF, FILE *stringF);
#endif

			uint16_t	index;
			char		name[ZUI_DRIVER_METALANGUAGE_MAXLEN];
		};

		struct sChildBop
		{
			uint16_t	metaIndex, regionIndex, opsIndex;
		};

		struct sParentBop
		{
			uint16_t	metaIndex, regionIndex, opsIndex,
					bindCbIndex;
		};

		struct sInternalBop
		{
			uint16_t	metaIndex, regionIndex,
					opsIndex0, opsIndex1, bindCbIndex;
		};

		struct Module
		{
			uint16_t	index;
			uint32_t	fileNameOff;
		};

		struct _Module
		{
#if !defined(__ZAMBESII_KERNEL_SOURCE__)
			int writeOut(FILE *dataF, FILE *stringF);
#endif

			uint16_t	index;
			char		fileName[ZUI_FILENAME_MAXLEN];
		};

		enum regionPrioE {
			REGION_PRIO_LOW=0, REGION_PRIO_MEDIUM, REGION_PRIO_HIGH };

		enum regionLatencyE {
			REGION_LAT_NON_CRITICAL=0, REGION_LAT_NON_OVER,
			REGION_LAT_RETRY, REGION_LAT_OVER,
			REGION_LAT_POWERFAIL_WARN };

		#define	ZUI_REGION_FLAGS_FP		(1<<0)
		#define	ZUI_REGION_FLAGS_DYNAMIC	(1<<1)
		#define	ZUI_REGION_FLAGS_INTERRUPT	(1<<2)
		struct sRegion
		{
#if !defined(__ZAMBESII_KERNEL_SOURCE__)
			int writeOut(FILE *dataF, FILE *stringF);
#endif

			uint32_t	driverId;
			uint16_t	index, moduleIndex;
			uint8_t		priority;
			uint8_t		latency;
			uint32_t	flags;
		};

		struct Message
		{
			uint32_t	driverId;
			uint16_t	index;
			uint32_t	messageOff;
		};

		struct _Message
		{
#if !defined(__ZAMBESII_KERNEL_SOURCE__)
			int writeOut(FILE *dataF, FILE *stringF);
#endif

			uint32_t	driverId;
			uint16_t	index;
			char		message[ZUI_MESSAGE_MAXLEN];
		};

		struct sDisasterMessage
		{
			uint32_t	driverId;
			uint16_t	index;
			uint32_t	messageOff;
		};

		struct _sDisasterMessage
		{
#if !defined(__ZAMBESII_KERNEL_SOURCE__)
			int writeOut(FILE *dataF, FILE *stringF);
#endif

			uint32_t	driverId;
			uint16_t	index;
			char		message[ZUI_MESSAGE_MAXLEN];
		};

		struct sMessageFile
		{
			uint32_t	driverId;
			uint16_t	index;
			uint32_t	fileNameOff;
		};

		struct _sMessageFile
		{
#if !defined(__ZAMBESII_KERNEL_SOURCE__)
			int writeOut(FILE *dataF, FILE *stringF);
#endif

			uint32_t	driverId;
			uint16_t	index;
			char		fileName[ZUI_FILENAME_MAXLEN];
		};

		struct sReadableFile
		{
			uint16_t	driverId, index;
			uint32_t	fileNameOff;
		};

		struct _sReadableFile
		{
#if !defined(__ZAMBESII_KERNEL_SOURCE__)
			int writeOut(FILE *dataF, FILE *stringF);
#endif

			uint16_t	driverId, index;
			char		fileName[ZUI_FILENAME_MAXLEN];
		};

		struct Provision
		{
			uint32_t	driverId;
			uint32_t	version;
			uint32_t	nameOff;
		};

		#define ZUI_PROVISION_NAME_MAXLEN	(ZUI_DRIVER_METALANGUAGE_MAXLEN)
		struct _Provision
		{
#if !defined(__ZAMBESII_KERNEL_SOURCE__)
			int writeOut(FILE *dataF, FILE *stringF);
#endif

			uint32_t	driverId;
			uint32_t	version;
			char		name[ZUI_PROVISION_NAME_MAXLEN];
		};

		struct Driver
		{
			struct zui::driver::sHeader	h;
			struct _sRequirement		requirements[
				ZUI_DRIVER_MAX_NREQUIREMENTS];

			struct _sMetalanguage		metalanguages[
				ZUI_DRIVER_MAX_NMETALANGUAGES];

			struct sChildBop		childBops[
				ZUI_DRIVER_MAX_NCHILD_BOPS];

			struct sParentBop		parentBops[
				ZUI_DRIVER_MAX_NPARENT_BOPS];

			struct sInternalBop		internalBops[
				ZUI_DRIVER_MAX_NINTERNAL_BOPS];

			struct _Module			modules[
				ZUI_DRIVER_MAX_NMODULES];
		};
	}

	namespace rank
	{
		#define ZUI_RANK_MAX_NATTRS		(ZUI_DEVICE_MAX_NATTRS)
		struct sHeader
		{
			uint32_t	driverId;
			uint8_t		nAttributes, rank;
			uint32_t	dataOff;
		};

		struct sRankAttr
		{
			uint32_t	nameOff;
		};

		struct _sRankAttr
		{
#if !defined(__ZAMBESII_KERNEL_SOURCE__)
			int writeOut(FILE *dataF, FILE *stringF);
#endif

			char		name[UDI_MAX_ATTR_NAMELEN];
		};

		struct _rankS
		{
#if !defined(__ZAMBESII_KERNEL_SOURCE__)
			int writeOut(FILE *rankF, FILE *dataF, FILE *stringF);
#endif

			struct zui::rank::sHeader	h;
			struct zui::rank::_sRankAttr	d[ZUI_RANK_MAX_NATTRS];
		};
	}
}

#endif

