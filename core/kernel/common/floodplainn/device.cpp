
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kstdlib/__kclib/stdlib.h>
#include <__kstdlib/__kcxxlib/memory>
#include <__kclasses/debugPipe.h>
#include <kernel/common/floodplainn/device.h>


error_t fplainn::driverC::preallocateModules(uarch_t nModules)
{
	modules = new moduleS[nModules];
	if (modules == NULL) { return ERROR_MEMORY_NOMEM; };
	this->nModules = nModules;
	return ERROR_SUCCESS;
}

error_t fplainn::driverC::preallocateRegions(uarch_t nRegions)
{
	regions = new regionS[nRegions];
	if (regions == NULL) { return ERROR_MEMORY_NOMEM; };
	this->nRegions = nRegions;
	return ERROR_SUCCESS;
}

error_t fplainn::driverC::preallocateRequirements(uarch_t nRequirements)
{
	requirements = new requirementS[nRequirements];
	if (requirements == NULL) { return ERROR_MEMORY_NOMEM; };
	this->nRequirements = nRequirements;
	return ERROR_SUCCESS;
}

error_t fplainn::driverC::preallocateMetalanguages(uarch_t nMetalanguages)
{
	metalanguages = new metalanguageS[nMetalanguages];
	if (metalanguages == NULL) { return ERROR_MEMORY_NOMEM; };
	this->nMetalanguages = nMetalanguages;
	return ERROR_SUCCESS;
}

error_t fplainn::driverC::preallocateChildBops(uarch_t nChildBops)
{
	childBops = new childBopS[nChildBops];
	if (childBops == NULL) { return ERROR_MEMORY_NOMEM; };
	this->nChildBops = nChildBops;
	return ERROR_SUCCESS;
}

error_t fplainn::driverC::preallocateParentBops(uarch_t nParentBops)
{
	parentBops = new parentBopS[nParentBops];
	if (parentBops == NULL) { return ERROR_MEMORY_NOMEM; };
	this->nParentBops = nParentBops;
	return ERROR_SUCCESS;
}

error_t fplainn::driverC::preallocateInternalBops(uarch_t nInternalBops)
{
	internalBops = new internalBopS[nInternalBops];
	if (internalBops == NULL) { return ERROR_MEMORY_NOMEM; };
	this->nInternalBops = nInternalBops;
	return ERROR_SUCCESS;
}

void fplainn::deviceC::dumpEnumerationAttributes(void)
{
	utf8Char		*fmtChar;

	printf(NOTICE"Device: @0x%p, id %d, %d enum attrs.\n\tlongname %s.\n",
		this, id, nEnumerationAttribs, longName);

	for (uarch_t i=0; i<nEnumerationAttribs; i++)
	{
		switch (enumeration[i]->attr_type)
		{
		case UDI_ATTR_STRING: fmtChar = CC"%s"; break;
		case UDI_ATTR_UBIT32: fmtChar = CC"0x%x"; break;
		case UDI_ATTR_ARRAY8: fmtChar = CC"%x"; break;
		case UDI_ATTR_BOOLEAN: fmtChar = CC"%x"; break;
		default:
			printf(ERROR"Unknown device attr type %d.\n",
				enumeration[i]->attr_type);
			return;
		};

		// This recursive %s feature is actually pretty nice.
		printf(NOTICE"Attr: name %s, type %d, val %r.\n",
			enumeration[i]->attr_name, enumeration[i]->attr_type,
			fmtChar,
			(!strcmp8(fmtChar, CC"%s"))
				? (void *)enumeration[i]->attr_value
				: (void *)(uintptr_t)UDI_ATTR32_GET(
					enumeration[i]->attr_value));
	};
}

error_t fplainn::deviceC::getEnumerationAttribute(
	utf8Char *name, udi_instance_attr_list_t *attrib
	)
{
	// Simple search for an attribute with the name supplied.
	for (uarch_t i=0; i<nEnumerationAttribs; i++)
	{
		if (strcmp8(CC enumeration[i]->attr_name, CC name) == 0)
		{
			*attrib = *enumeration[i];
			return ERROR_SUCCESS;
		};
	};

	return ERROR_NOT_FOUND;
}

error_t fplainn::deviceC::setEnumerationAttribute(
	udi_instance_attr_list_t *attrib
	)
{
	error_t					ret;
	udi_instance_attr_list_t		**attrArrayMem, **tmp;
	heapPtrC<udi_instance_attr_list_t>	destAttrMem;
	sarch_t					releaseAttrMem=0;

	if (attrib == NULL) { return ERROR_INVALID_ARG; };

	destAttrMem = new udi_instance_attr_list_t;
	if (destAttrMem == NULL) { return ERROR_MEMORY_NOMEM; };

	// Can do other checks, such as checks on supplied attr's "type" etc.

	// Check to see if the attrib already exists.
	ret = getEnumerationAttribute(CC attrib->attr_name, destAttrMem.get());
	if (ret != ERROR_SUCCESS)
	{
		// Allocate mem for the new array of pointers to attribs.
		attrArrayMem = new udi_instance_attr_list_t *[
			nEnumerationAttribs + 1];

		if (attrArrayMem == NULL) { return ERROR_MEMORY_NOMEM; };

		if (nEnumerationAttribs > 0)
		{
			memcpy(
				attrArrayMem, enumeration,
				sizeof(*enumeration) * nEnumerationAttribs);
		};

		attrArrayMem[nEnumerationAttribs] = destAttrMem.get();
		/* Since we have to use the allocated memory to store the new
		 * attribute permanently, we have to call release() on the
		 * pointer management object later on. Set this bool to indicate
		 * that.
		 **/
		releaseAttrMem = 1;
		tmp = enumeration;
		enumeration = attrArrayMem;
		nEnumerationAttribs++;

		delete tmp;
	};

	memcpy(destAttrMem.get(), attrib, sizeof(*attrib));
	if (releaseAttrMem) { destAttrMem.release(); };
	return ERROR_SUCCESS;
}

error_t fplainn::driverC::moduleS::addAttachedRegion(ubit16 regionIndex)
{
	regionIndexes = new (realloc(regionIndexes, nAttachedRegions + 1))
		ubit16;

	if (regionIndexes == NULL) { return ERROR_MEMORY_NOMEM; };
	regionIndexes[nAttachedRegions++] = regionIndex;
	return ERROR_SUCCESS;
}

void fplainn::driverC::dump(void)
{
	printf(NOTICE"Driver: %s/%s\n\t(%s).\n\tSupplier %s; Contact %s.\n"
		"%d mods, %d rgns, %d req's, %d metas, %d cbops, %d pbops, "
		"%d ibops.\n",
		basePath, shortName, longName, supplier, supplierContact,
		nModules, nRegions, nRequirements, nMetalanguages,
		nChildBops, nParentBops, nInternalBops);
}

