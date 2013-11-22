
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <kernel/common/floodplainn/device.h>


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
	utf8Char *name, udi_instance_attr_list_t **attrib
	)
{
	// Simple search for an attribute with the name supplied.
	for (uarch_t i=0; i<nEnumerationAttribs; i++)
	{
		if (strcmp8(CC enumeration[i]->attr_name, CC name) == 0)
		{
			*attrib = enumeration[i];
			return ERROR_SUCCESS;
		};
	};

	return ERROR_NOT_FOUND;
}

error_t fplainn::deviceC::setEnumerationAttribute(
	udi_instance_attr_list_t *attrib
	)
{
	error_t				ret;
	udi_instance_attr_list_t	*destAttrMem, **attrArrayMem, **tmp;

	if (attrib == NULL) { return ERROR_INVALID_ARG; };

	// Can do other checks, such as checks on supplied attr's "type" etc.

	// Check to see if the attrib already exists.
	ret = getEnumerationAttribute(CC attrib->attr_name, &destAttrMem);
	if (ret != ERROR_SUCCESS)
	{
		// If it doesn't exist, allocate mem for a new attribute.
		destAttrMem = new udi_instance_attr_list_t;
		if (destAttrMem == NULL) { return ERROR_MEMORY_NOMEM; };

		// And also mem to resize the array of pointers.
		attrArrayMem = new udi_instance_attr_list_t *[
			nEnumerationAttribs + 1];

		if (attrArrayMem == NULL)
			{ delete destAttrMem; return ERROR_MEMORY_NOMEM; };

		if (nEnumerationAttribs > 0)
		{
			memcpy(
				attrArrayMem, enumeration,
				sizeof(*enumeration) * nEnumerationAttribs);
		};

		attrArrayMem[nEnumerationAttribs] = destAttrMem;
		tmp = enumeration;
		enumeration = attrArrayMem;
		nEnumerationAttribs++;

		delete tmp;
	};

	memcpy(destAttrMem, attrib, sizeof(*attrib));
	return ERROR_SUCCESS;
}

error_t fplainn::driverC::addModule(ubit16 index, utf8Char *filename)
{
	moduleC		*old;

	if (strnlen8(filename, ZUDI_FILENAME_MAXLEN) >= ZUDI_FILENAME_MAXLEN)
		{ return ERROR_INVALID_RESOURCE_NAME; };

	old = modules;
	modules = new moduleC[nModules + 1];
	if (modules == NULL) { return ERROR_MEMORY_NOMEM; };

	if (nModules > 0)
		{ memcpy(modules, old, sizeof(*old) * nModules); };

	new (&modules[nModules]) moduleC(filename, index);

	// Now add the regions that pertain to this module.
	// for () {};

	return ERROR_SUCCESS;
}

