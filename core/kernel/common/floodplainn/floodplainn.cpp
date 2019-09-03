
#include <arch/debug.h>
#include <chipset/chipset.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kstdlib/__kcxxlib/memory>
#include <__kclasses/debugPipe.h>
#include <kernel/common/floodplainn/floodplainn.h>
#include <kernel/common/floodplainn/zudi.h>
#include <kernel/common/taskTrib/taskTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/vfsTrib/vfsTrib.h>

ubit8		fplainn::dma::nullSGList[
	sizeof(fplainn::dma::ScatterGatherList)];

udi_dma_constraints_attr_spec_t _defaultConstraints[] = {
	{ UDI_DMA_SCGTH_FORMAT, UDI_SCGTH_32
#if __PADDR_NBITS__ <= 64 && __PADDR_NBITS__ > 32
				| UDI_SCGTH_64
#endif
#ifdef __ARCH_LITTLE_ENDIAN__
				| UDI_DMA_LITTLE_ENDIAN
#elif defined(__ARCH_BIG_ENDIAN__)
				| UDI_DMA_BIG_ENDIAN
#else
	#error "Unable to determine endianness of target host CPU arch!"
#endif
	},
	{ UDI_DMA_ADDRESSABLE_BITS, __PADDR_NBITS__ },
	{ UDI_DMA_ALIGNMENT_BITS, PAGING_BASE_SHIFT }
};

fplainn::dma::constraints::Compiler	FloodplainnStream::defaultConstraints;

static fplainn::Device		byId(CHIPSET_NUMA_SHBANKID),
				byName(CHIPSET_NUMA_SHBANKID),
				byClass(CHIPSET_NUMA_SHBANKID),
				byPath(CHIPSET_NUMA_SHBANKID);

error_t Floodplainn::initialize(void)
{
	fvfs::Tag				*root, *tmp;
	error_t					ret;
	udi_instance_attr_list_t		tmpAttr;
	fplainn::dma::Constraints		constraintsParser;

	/**	EXPLANATION:
	 * Create the four sub-trees and set up the enumeration attributes of
	 * the root device ("@f/by-id") and then exit.
	 *
	 * The root device node (@f/by-id) must be given enumeration attributes
	 * which will cause it to be detected as the root device by the ZUI
	 * Server.
	 *
	 * Basically, the ZUI Server will then instantiate the root device
	 * driver, which will then enumerate the first child nodes of the by-id
	 * tree, namely @f/by-id/0 and @f/by-id/1, which are the __kramdisk
	 * and vchipset devices respectively.
	 **/
	ret = byId.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };
	ret = byName.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };
	ret = byClass.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };
	ret = byPath.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

	root = vfsTrib.getFvfs()->getRoot();

	ret = root->getInode()->createChild(
		CC"by-id", root, &byId, CC"zbz_fplainn", &tmp);
	if (ret != ERROR_SUCCESS) { return ret; };
	ret = root->getInode()->createChild(
		CC"by-name", root, &byName, CC"zbz_fplainn", &tmp);
	if (ret != ERROR_SUCCESS) { return ret; };
	ret = root->getInode()->createChild(
		CC"by-class", root, &byClass, CC"zbz_fplainn", &tmp);
	if (ret != ERROR_SUCCESS) { return ret; };
	ret = root->getInode()->createChild(
		CC"by-path", root, &byPath, CC"zbz_fplainn", &tmp);
	if (ret != ERROR_SUCCESS) { return ret; };

	/* Create the enumeration attributes for the root device.
	 */
	strcpy8(CC tmpAttr.attr_name, CC"identifier");
	strcpy8(CC tmpAttr.attr_value, CC"zrootdev");
	tmpAttr.attr_type = UDI_ATTR_STRING;
	ret = byId.addEnumerationAttribute(&tmpAttr);
	if (ret != ERROR_SUCCESS) { return ret; };

	strcpy8(CC tmpAttr.attr_name, CC"address_locator");
	strcpy8(CC tmpAttr.attr_value, CC"zrootdev0");
	tmpAttr.attr_type = UDI_ATTR_STRING;
	ret = byId.addEnumerationAttribute(&tmpAttr);
	if (ret != ERROR_SUCCESS) { return ret; };

	strcpy8(CC tmpAttr.attr_name, CC"physical_locator");
	strcpy8(CC tmpAttr.attr_value, CC"zrootdev0");
	tmpAttr.attr_type = UDI_ATTR_STRING;
	ret = byId.addEnumerationAttribute(&tmpAttr);
	if (ret != ERROR_SUCCESS) { return ret; };

	strcpy8(CC tmpAttr.attr_name, CC"physical_label");
	strcpy8(CC tmpAttr.attr_value, CC"in memory");
	tmpAttr.attr_type = UDI_ATTR_STRING;
	ret = byId.addEnumerationAttribute(&tmpAttr);
	if (ret != ERROR_SUCCESS) { return ret; };

	memset(
		fplainn::dma::nullSGList, 0,
		sizeof(sizeof(fplainn::dma::ScatterGatherList)));

	ret = constraintsParser.initialize(_defaultConstraints, 2);
	if (ret != ERROR_SUCCESS)
	{
		printf(ERROR FPLAINN"Failed to initialize parser for default "
			"DMA constraints initialization.\n");
		return ret;
	}

	ret = FloodplainnStream::defaultConstraints.initialize();
	if (ret != ERROR_SUCCESS)
	{
		printf(ERROR FPLAINN"Failed to initialize constraints compiler "
			"for default constraints.\n");
		return ret;
	}

	ret = FloodplainnStream::defaultConstraints.compile(&constraintsParser);
	if (ret != ERROR_SUCCESS)
	{
		printf(ERROR FPLAINN"Failed to compile default DMA "
			"Constraints.\n");
		return ret;
	}

	/* Set up by-id's default constraints to be the chipset's default
	 * constraints.
	 */
	byId.getParentTag(1)->compiledConstraints.rsrc =
		FloodplainnStream::defaultConstraints;

	return ERROR_SUCCESS;
}

static inline sarch_t isByIdPath(utf8Char *path)
{
	// Quick and dirty string parser to ensure the path is a by-id path.
	for (; *path == '/'; path++) {};
	if (strncmp8(path, CC"@f/", 3) == 0) { path += 3; };
	for (; *path == '/'; path++) {};
	if (strncmp8(path, CC"by-id", 5) != 0) { return 0; };
	return 1;
}

error_t Floodplainn::createDevice(
	utf8Char *parentId, numaBankId_t bid, ubit16 childId,
	utf8Char *enumeratingMetaName,
	ubit32 /*flags*/, fplainn::Device **device
	)
{
	error_t			ret;
	fvfs::Tag		*parentTag, *childTag;
	fplainn::Device		*newDev;

	/**	EXPLANATION:
	 * This function both creates the device, and creates its nodes in the
	 * by-id, by-path and by-name trees.
	 *
	 * The path here must be a by-id path.
	 **/
	if (parentId == NULL || device == NULL)
		{ return ERROR_INVALID_ARG; };

	if (!isByIdPath(parentId)) { return ERROR_INVALID_ARG_VAL; };

	// Get the parent node's VFS tag.
	ret = vfsTrib.getFvfs()->getPath(parentId, &parentTag);
	if (ret != ERROR_SUCCESS) { return ret; };

	newDev = new fplainn::Device(bid);
	if (newDev == NULL) { return ERROR_MEMORY_NOMEM; };
	ret = newDev->initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

	utf8Char	tmpBuff[FVFS_TAG_NAME_MAXLEN];

	// Add the new device node to the parent node.
	snprintf(tmpBuff, FVFS_TAG_NAME_MAXLEN, CC"%d", childId);
	ret = parentTag->getInode()->createChild(
		tmpBuff, parentTag, newDev, enumeratingMetaName, &childTag);

	if (ret != ERROR_SUCCESS) { delete newDev; };
	*device = newDev;
	return ERROR_SUCCESS;
}

error_t Floodplainn::removeDevice(
	utf8Char *parentId, ubit32 childId, ubit32 /*flags*/
	)
{
	fvfs::Tag		*parentTag, *childTag;
	error_t			ret;

	/**	EXPLANATION:
	 * parentId must be a by-id path.
	 **/

	if (parentId == NULL) { return ERROR_INVALID_ARG; };

	if (!isByIdPath(parentId)) { return ERROR_INVALID_ARG_VAL; };
	ret = vfsTrib.getFvfs()->getPath(parentId, &parentTag);
	if (ret != ERROR_SUCCESS) { return ret; };

	utf8Char	tmpBuff[FVFS_TAG_NAME_MAXLEN];

	// Get the child tag.
	snprintf(tmpBuff, FVFS_TAG_NAME_MAXLEN, CC"%d", childId);
	childTag = parentTag->getInode()->getChild(tmpBuff);
	if (childTag == NULL) { return ERROR_NOT_FOUND; };

	// Don't remove the child if it has children.
	if (childTag->getInode()->dirs.getNItems() > 0)
		{ return ERROR_INVALID_OPERATION; };

	// Delete the device object it points to, then delete the child tag.
	delete childTag->getInode();
	parentTag->getInode()->removeChild(tmpBuff);

	return ERROR_SUCCESS;
}

error_t Floodplainn::getDevice(utf8Char *path, fplainn::Device **device)
{
	error_t		ret;
	fvfs::Tag	*tag;

	ret = vfsTrib.getFvfs()->getPath(path, &tag);
	if (ret != ERROR_SUCCESS) { return ret; };

	/* Do not allow callers to "getDevice" on the by-* containers.
	 * e.g: "by-name", "/by-name", "@f/by-name", etc.
	 *
	 * The only one callers are permitted to get is by-id, since it is
	 * also the root device.
	 **/
	if (tag->getInode() == &byName || tag->getInode() == &byClass
		|| tag->getInode() == &byPath)
		{ return ERROR_UNAUTHORIZED; };

	*device = tag->getInode();
	return ERROR_SUCCESS;
}

error_t Floodplainn::enumerateBaseDevices(void)
{
	error_t				ret;
	fplainn::Device			*vchipset, *ramdisk;
	udi_instance_attr_list_t	tmp;

	ret = createDevice(
		CC"by-id", CHIPSET_NUMA_SHBANKID, 0, CC"udi_bridge", 0, &ramdisk);

	if (ret != ERROR_SUCCESS) { return ret; };

	ret = createDevice(
		CC"by-id", CHIPSET_NUMA_SHBANKID, 1, CC"udi_bridge", 0,
		&vchipset);

	if (ret != ERROR_SUCCESS) { return ret; };

	/* Create the attributes for the ramdisk.
	 */
	strcpy8(CC tmp.attr_name, CC"identifier");
	tmp.attr_type = UDI_ATTR_STRING;
	strcpy8(CC tmp.attr_value, CC"__kramdisk");
	ret = ramdisk->addEnumerationAttribute(&tmp);
	if (ret != ERROR_SUCCESS) { return ret; };

	strcpy8(CC tmp.attr_name, CC"address_locator");
	tmp.attr_type = UDI_ATTR_STRING;
	strcpy8(CC tmp.attr_value, CC"__kramdisk0");
	ret = ramdisk->addEnumerationAttribute(&tmp);
	if (ret != ERROR_SUCCESS) { return ret; };

	/* Create the attributes for the virtual-chipset.
	 */
	strcpy8(CC tmp.attr_name, CC"identifier");
	tmp.attr_type = UDI_ATTR_STRING;
	strcpy8(CC tmp.attr_value, CC"__kvirtual-chipset");
	ret = vchipset->addEnumerationAttribute(&tmp);
	if (ret != ERROR_SUCCESS) { return ret; };

	strcpy8(CC tmp.attr_name, CC"address_locator");
	tmp.attr_type = UDI_ATTR_STRING;
	strcpy8(CC tmp.attr_value, CC"__kvchipset0");
	ret = vchipset->addEnumerationAttribute(&tmp);
	if (ret != ERROR_SUCCESS) { return ret; };

	ramdisk->bankId = vchipset->bankId = CHIPSET_NUMA_SHBANKID;
	return ERROR_SUCCESS;
}

error_t Floodplainn::enumerateChipsets(void)
{
	error_t				ret;
	fplainn::Device		*chipset;
	udi_instance_attr_list_t	tmp;

	ret = createDevice(
		CC"by-id", CHIPSET_NUMA_SHBANKID, 2, CC"udi_bridge", 0,
		&chipset);

	if (ret != ERROR_SUCCESS) { return ret; };

	/* Generic enumeration attributes.
	 **/
	strcpy8(CC tmp.attr_name, CC"identifier");
	tmp.attr_type = UDI_ATTR_STRING;
	strcpy8(CC tmp.attr_value, CC"ibm-pc");
	ret = chipset->addEnumerationAttribute(&tmp);
	if (ret != ERROR_SUCCESS) { return ret; };

	strcpy8(CC tmp.attr_name, CC"address_locator");
	tmp.attr_type = UDI_ATTR_STRING;
	strcpy8(CC tmp.attr_value, CC"chipset0");
	ret = chipset->addEnumerationAttribute(&tmp);
	if (ret != ERROR_SUCCESS) { return ret; };

	/* zbz_root enumeration attributes.
	 **/
	strcpy8(CC tmp.attr_name, CC"bus_type");
	tmp.attr_type = UDI_ATTR_STRING;
	strcpy8(CC tmp.attr_value, CC"zbz_root");
	ret = chipset->addEnumerationAttribute(&tmp);
	if (ret != ERROR_SUCCESS) { return ret; };

	strcpy8(CC tmp.attr_name, CC"zbz_root_chipset_short_name");
	tmp.attr_type = UDI_ATTR_STRING;
	// strcpy8(CC tmp.attr_value, CC CHIPSET_SHORT_STRING);
	strcpy8(CC tmp.attr_value, CC"IBM-PC");
	ret = chipset->addEnumerationAttribute(&tmp);
	if (ret != ERROR_SUCCESS) { return ret; };

	strcpy8(CC tmp.attr_name, CC"zbz_root_chipset_long_name");
	tmp.attr_type = UDI_ATTR_STRING;
	// strcpy8(CC tmp.attr_value, CC CHIPSET_LONG_STRING);
	strcpy8(CC tmp.attr_value, CC"IBM-PC-compatible-chipset");
	ret = chipset->addEnumerationAttribute(&tmp);
	if (ret != ERROR_SUCCESS) { return ret; };

	strcpy8(CC tmp.attr_name, CC"zbz_root_chipset_using_smp");
	tmp.attr_type = UDI_ATTR_BOOLEAN;
	tmp.attr_value[0] = cpuTrib.usingChipsetSmpMode();
	ret = chipset->addEnumerationAttribute(&tmp);
	if (ret != ERROR_SUCCESS) { return ret; };

	strcpy8(CC tmp.attr_name, CC"zbz_root_chipset_info0");
	tmp.attr_type = UDI_ATTR_STRING;
	tmp.attr_value[0] = '\0';
	ret = chipset->addEnumerationAttribute(&tmp);
	if (ret != ERROR_SUCCESS) { return ret; };

	strcpy8(CC tmp.attr_name, CC"zbz_root_chipset_info1");
	tmp.attr_type = UDI_ATTR_STRING;
	tmp.attr_value[0] = '\0';
	ret = chipset->addEnumerationAttribute(&tmp);

	chipset->bankId = CHIPSET_NUMA_SHBANKID;
	return ERROR_SUCCESS;
}
