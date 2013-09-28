#ifndef _Z_UDI_INDEX_H
	#define _Z_UDI_INDEX_H

	#include <stdint.h>

/**	EXPLANATION:
 * This header is contained in all UDI index files. The kernel uses it to
 * quickly gain information about the whole of the index at a glance. Versioning
 * of the struct layouts used is also included for forward expansion.
 **/
#define ZUDI_MESSAGE_MAXLEN		(150)
#define ZUDI_FILENAME_MAXLEN		(64)

struct zudiIndexHeaderS
{
	// Version of the record format used in this index file.
	// "endianness" is a NULL-terminated string of either "le" or "be".
	char		endianness[4];
	uint16_t	majorVersion, minorVersion;
	uint32_t	nRecords, nextDriverId;
	uint8_t		reserved[64];
};

enum zudiIndexDeviceAttrTypeE {
	ZUDI_DEVICE_ATTR_STRING=0, ZUDI_DEVICE_ATTR_UBIT32,
	ZUDI_DEVICE_ATTR_BOOL, ZUDI_DEVICE_ATTR_ARRAY8 };

#define ZUDI_DEVICE_MAX_NATTRIBS		(20)
#define ZUDI_DEVICE_ATTRIB_NAME_MAXLEN		(32)
#define ZUDI_DEVICE_ATTRIB_VALUE_MAXLEN		(64)
struct zudiIndexDeviceS
{
	struct zudiIndexDeviceHeaderS
	{
		uint32_t	driverId;
		uint16_t	index;
		uint16_t	messageIndex, metaIndex;
		uint8_t		nAttributes;
		uint32_t	dataFileOffset;
	} h;

	struct zudiIndexDeviceDataS
	{
		uint8_t		type, size;
		char		name[ZUDI_DEVICE_ATTRIB_NAME_MAXLEN];
		union
		{
			char	string[ZUDI_DEVICE_ATTRIB_VALUE_MAXLEN];
			uint8_t	array8[ZUDI_DEVICE_ATTRIB_VALUE_MAXLEN];
			uint32_t	unsigned32;
			uint8_t		boolval;
		} value;
	} d[ZUDI_DEVICE_MAX_NATTRIBS];
};

#define ZUDI_DRIVER_MAX_NREQUIREMENTS		(16)
#define ZUDI_DRIVER_MAX_NMETALANGUAGES		(16)
#define ZUDI_DRIVER_MAX_NCHILD_BOPS		(12)
#define ZUDI_DRIVER_MAX_NPARENT_BOPS		(8)
#define ZUDI_DRIVER_MAX_NINTERNAL_BOPS		(24)
#define ZUDI_DRIVER_MAX_NMODULES		(16)

#define ZUDI_DRIVER_SHORTNAME_MAXLEN		(16)
#define ZUDI_DRIVER_RELEASE_MAXLEN		(32)
#define ZUDI_DRIVER_BASEPATH_MAXLEN		(128)
#define ZUDI_DRIVER_METALANGUAGE_MAXLEN		(32)
#define ZUDI_DRIVER_REQUIREMENT_MAXLEN		\
					(ZUDI_DRIVER_METALANGUAGE_MAXLEN)

enum zudiIndexDriverTypeE	{ DRIVERTYPE_DRIVER, DRIVERTYPE_METALANGUAGE };
struct zudiIndexDriverS
{
	struct zudiIndexDriverHeaderS
	{
		// TODO: Add support for custom attributes.
		uint32_t	id, type;
		uint16_t	nameIndex, supplierIndex, contactIndex,
		// Category index is only valid for meta libs, not drivers.
				categoryIndex;
		char		shortName[ZUDI_DRIVER_SHORTNAME_MAXLEN];
		char		releaseString[ZUDI_DRIVER_RELEASE_MAXLEN];
		char		releaseStringIndex;
		uint32_t	requiredUdiVersion;
		char		basePath[ZUDI_DRIVER_BASEPATH_MAXLEN];

		/* dataFileOffset is the offset within driver-data.zudi-index.
		 * rankFileOffset is the offset within ranks.zudi-index.
		 **/
		uint32_t	dataFileOffset, rankFileOffset,
				deviceFileOffset;
		uint8_t		nMetalanguages, nChildBops, nParentBops,
				nInternalBops,
				nModules, nRequirements,
				nMessages, nDisasterMessages,
				nMessageFiles, nReadableFiles, nRegions,
				nDevices, nRanks, nProvides;
	} h;

	struct zudiIndexDriverDataS
	{
		struct
		{
			uint32_t	version;
			char		name[ZUDI_DRIVER_REQUIREMENT_MAXLEN];
		} requirements[ZUDI_DRIVER_MAX_NREQUIREMENTS];

		struct
		{
			uint16_t	index;
			char		name[ZUDI_DRIVER_METALANGUAGE_MAXLEN];
		} metalanguages[ZUDI_DRIVER_MAX_NMETALANGUAGES];

		struct
		{
			uint16_t	metaIndex, regionIndex, opsIndex;
		} childBops[ZUDI_DRIVER_MAX_NCHILD_BOPS];

		struct
		{
			uint16_t	metaIndex, regionIndex, opsIndex,
					bindCbIndex;
		} parentBops[ZUDI_DRIVER_MAX_NPARENT_BOPS];

		struct
		{
			uint16_t	metaIndex, regionIndex,
					opsIndex0, opsIndex1, bindCbIndex;
		} internalBops[ZUDI_DRIVER_MAX_NINTERNAL_BOPS];

		struct
		{
			uint16_t	index;
			char		fileName[ZUDI_FILENAME_MAXLEN];
		} modules[ZUDI_DRIVER_MAX_NMODULES];
	} d;
};

enum zudiIndexRegionPrioE	{
	ZUDI_REGION_PRIO_LOW=0, ZUDI_REGION_PRIO_MEDIUM,
	ZUDI_REGION_PRIO_HIGH };

enum zudiIndexRegionLatencyE {
	ZUDI_INDEX_REGIONLAT_NON_CRITICAL=0, ZUDI_INDEX_REGIONLAT_NON_OVER,
	ZUDI_INDEX_REGIONLAT_RETRY, ZUDI_INDEX_REGIONLAT_OVER,
	ZUDI_INDEX_REGIONLAT_POWERFAIL_WARN };

#define	ZUDI_REGION_FLAGS_FP		(1<<0)
#define	ZUDI_REGION_FLAGS_DYNAMIC	(1<<1)
#define	ZUDI_REGION_FLAGS_INTERRUPT	(1<<2)
struct zudiIndexRegionS
{
	uint32_t	driverId;
	uint16_t	index, moduleIndex;
	uint8_t		priority;
	uint8_t		latency;
	uint32_t	flags;
};

struct zudiIndexMessageS
{
	uint32_t	driverId;
	uint16_t	index;
	char		message[ZUDI_MESSAGE_MAXLEN];
};

struct zudiIndexDisasterMessageS
{
	uint32_t	driverId;
	uint16_t	index;
	char		message[ZUDI_MESSAGE_MAXLEN];
};

struct zudiIndexMessageFileS
{
	uint32_t	driverId;
	uint16_t	index;
	char		fileName[ZUDI_FILENAME_MAXLEN];
};

struct zudiIndexReadableFileS
{
	uint16_t	driverId, index;
	char		fileName[ZUDI_FILENAME_MAXLEN];
};

#define ZUDI_PROVISION_NAME_MAXLEN	(ZUDI_DRIVER_METALANGUAGE_MAXLEN)
struct zudiIndexProvisionS
{
	uint32_t	driverId;
	uint32_t	version;
	char		name[ZUDI_PROVISION_NAME_MAXLEN];
};

#define ZUDI_RANK_MAX_NATTRIBS		(ZUDI_DEVICE_MAX_NATTRIBS)
#define ZUDI_RANK_ATTRIB_NAME_MAXLEN	(ZUDI_DEVICE_ATTRIB_NAME_MAXLEN)
struct zudiIndexRankS
{
	struct zudiIndexRankHeaderS
	{
		uint32_t	driverId;
		uint8_t		nAttributes, rank;
	} h;

	struct zudiIndexRankDataS
	{
		char		name[ZUDI_RANK_ATTRIB_NAME_MAXLEN];
	} d[ZUDI_RANK_MAX_NATTRIBS];
};

#endif

