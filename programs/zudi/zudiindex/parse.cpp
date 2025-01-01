
#include "zudipropsc.h"
#include <stdio.h>
#include <string.h>


/**	EXPLANATION:
 * Very simple stateful parser. It will parse one udiprops at a time (therefore
 * one driver at a time), so we can just keep a single global pointer and
 * allocate memory for it on each new call to parser_initializeNewDriver().
 **/
struct zui::driver::sDriver	*currentDriver=NULL;
const char			*limitExceededMessage=
	"Limit exceeded for entity";

int parser_initializeNewDriverState(uint16_t driverId)
{
	/**	EXPLANATION:
	 * Causes the parser to allocate a new driver object and delete the old
	 * parsed state.
	 *
	 * If a driver was parsed previously, the caller is expected to first
	 * use parser_getCurrentDriverState() to get the pointer to the old
	 * state if it needs it, before calling this function.
	 **/
	if (currentDriver != NULL)
	{
		free(currentDriver);
		currentDriver = NULL;
	};

	currentDriver = new zui::driver::sDriver;
	if (currentDriver == NULL) { return 0; };

	memset(currentDriver, 0, sizeof(*currentDriver));
	currentDriver->h.id = driverId;
	strcpy(currentDriver->h.basePath, basePath);
	if (propsType == META_PROPS) {
		currentDriver->h.type = zui::driver::DRIVERTYPE_METALANGUAGE;
	} else {
		currentDriver->h.type = zui::driver::DRIVERTYPE_DRIVER;
	};

	return 1;
}

struct zui::driver::sDriver *parser_getCurrentDriverState(void)
{
	return currentDriver;
}

int parser_getNSupportedDevices(void)
{
	return currentDriver->h.nDevices;
}

int parser_getNSupportedMetas(void)
{
	return currentDriver->h.nProvisions;
}

void parser_releaseState(void)
{
	if (currentDriver != NULL)
	{
		free(currentDriver);
		currentDriver = NULL;
	};
}

static const char *skipWhitespaceIn(const char *str)
{
	for (; *str == ' ' || *str == '\t'; str++) {};
	return str;
}

static char *findWhitespaceAfter(const char *str)
{
	for (; *str; str++) {
		if (*str == ' ' || *str == '\t') { return (char *)str; };
	};

	return NULL;
}

static size_t strlenUpToWhitespace(const char *str, const char *white)
{
	if (white == NULL) {
		return strlen(str);
	}

	return white - str;
}

static void strcpyUpToWhitespace(
	char *dest, const char *source, const char *white
	)
{
	if (white != NULL)
	{
		strncpy(dest, source, white - source);
		dest[white - source] = '\0';
	}
	else { strcpy(dest, source); };
}

static inline int hasSlashes(const char *str)
{
	if (strchr(str, '/') != NULL) { return 1; };
	return 0;
}

#define PARSER_MALLOC(__varPtr, __type)		do \
	{ \
		*__varPtr = (__type *)malloc(sizeof(__type)); \
		if (*__varPtr == NULL) \
			{ printf("Malloc failed.\n"); return NULL; }; \
		memset(*__varPtr, 0, sizeof(__type)); \
	} while (0)

#define PARSER_RELEASE_AND_EXIT(__varPtr) \
	releaseAndExit: \
		free(*__varPtr); \
		return NULL

static void *parseMessage(const char *line)
{
	struct zui::driver::_sMessage	*ret;
	char				*tmp;

	PARSER_MALLOC(&ret, struct zui::driver::_sMessage);
	line = skipWhitespaceIn(line);

	ret->index = strtoul(line, &tmp, 10);
	// msgnum index 0 is reserved by the UDI specification.
	if (line == tmp || ret->index == 0) { goto releaseAndExit; };

	line = skipWhitespaceIn(tmp);

	if (strlen(line) >= ZUI_MESSAGE_MAXLEN) { goto releaseAndExit; };
	strcpy(ret->message, line);
	ret->driverId = currentDriver->h.id;

	if (verboseMode)
	{
		sprintf(
			verboseBuff, "MESSAGE(%05d): \"%s\"",
			ret->index, ret->message);
	};

	currentDriver->h.nMessages++;
	return ret;
PARSER_RELEASE_AND_EXIT(&ret);
}

static void *parseDisasterMessage(const char *line)
{
	struct zui::driver::_sDisasterMessage	*ret;
	char					*tmp;

	PARSER_MALLOC(&ret, struct zui::driver::_sDisasterMessage);
	line = skipWhitespaceIn(line);

	ret->index = strtoul(line, &tmp, 10);
	// msgnum index 0 is reserved by the UDI specification.
	if (line == tmp || ret->index == 0) { goto releaseAndExit; };

	line = skipWhitespaceIn(tmp);

	if (strlen(line) >= ZUI_MESSAGE_MAXLEN) { goto releaseAndExit; };
	strcpy(ret->message, line);
	ret->driverId = currentDriver->h.id;

	if (verboseMode)
	{
		sprintf(
			verboseBuff, "DISASTER_MESSAGE(%02d): \"%s\"",
			ret->index, ret->message);
	};

	currentDriver->h.nDisasterMessages++;
	return ret;
PARSER_RELEASE_AND_EXIT(&ret);
}

static void *parseMessageFile(const char *line)
{
	struct zui::driver::_sMessageFile	*ret;

	PARSER_MALLOC(&ret, struct zui::driver::_sMessageFile);
	line = skipWhitespaceIn(line);

	if (strlen(line) >= ZUI_FILENAME_MAXLEN) { goto releaseAndExit; };

	if (hasSlashes(line)) { goto releaseAndExit; };
	strcpy(ret->fileName, line);
	ret->driverId = currentDriver->h.id;
	ret->index = currentDriver->h.nMessageFiles;

	if (verboseMode) {
		sprintf(verboseBuff, "MESSAGE_FILE: \"%s\"", ret->fileName);
	};

	currentDriver->h.nMessageFiles++;
	return ret;
PARSER_RELEASE_AND_EXIT(&ret);
}

static void *parseReadableFile(const char *line)
{
	struct zui::driver::_sReadableFile	*ret;

	PARSER_MALLOC(&ret, struct zui::driver::_sReadableFile);
	line = skipWhitespaceIn(line);

	if (strlen(line) >= ZUI_FILENAME_MAXLEN) { goto releaseAndExit; };

	if (hasSlashes(line)) { goto releaseAndExit; };
	strcpy(ret->fileName, line);
	ret->driverId = currentDriver->h.id;
	ret->index = currentDriver->h.nReadableFiles;

	if (verboseMode) {
		sprintf(verboseBuff, "READABLE_FILE: \"%s\"", ret->fileName);
	};

	currentDriver->h.nReadableFiles++;
	return ret;
PARSER_RELEASE_AND_EXIT(&ret);
}

static int parseShortName(const char *line)
{
	const char	*white;

	line = skipWhitespaceIn(line);
	white = findWhitespaceAfter(line);

	if (strlenUpToWhitespace(line, white) >= ZUI_DRIVER_SHORTNAME_MAXLEN)
		{ return 0; };

	// No whitespace is allowed in the shortname.
	strcpyUpToWhitespace(currentDriver->h.shortName, line, white);

	if (verboseMode)
	{
		sprintf(verboseBuff, "SHORT_NAME: \"%s\"",
			currentDriver->h.shortName);
	};

	return 1;
}

static int parseSupplier(const char *line)
{
	char		*tmp;

	line = skipWhitespaceIn(line);
	// strtol returns 0 when it fails to convert.
	currentDriver->h.supplierIndex = strtoul(line, &tmp, 10);
	if (line == tmp || currentDriver->h.supplierIndex == 0) { return 0; };

	if (verboseMode)
	{
		sprintf(verboseBuff, "SUPPLIER: %d",
			currentDriver->h.supplierIndex);
	};

	return 1;
}

static int parseContact(const char *line)
{
	char		*tmp;

	line = skipWhitespaceIn(line);
	// strtol returns 0 when it fails to convert.
	currentDriver->h.contactIndex = strtoul(line, &tmp, 10);
	if (line == tmp || currentDriver->h.contactIndex == 0) { return 0; };

	if (verboseMode)
	{
		sprintf(verboseBuff, "CONTACT: %d",
			currentDriver->h.contactIndex);
	};

	return 1;
}

static int parseName(const char *line)
{
	char		*tmp;

	line = skipWhitespaceIn(line);
	// strtol returns 0 when it fails to convert.
	currentDriver->h.nameIndex = strtoul(line, &tmp, 10);
	if (line == tmp || currentDriver->h.nameIndex == 0) { return 0; };

	if (verboseMode)
	{
		sprintf(verboseBuff, "NAME: %d",
			currentDriver->h.nameIndex);
	};

	return 1;
}

static int parseRelease(const char *line)
{
	char		*tmp;

	line = skipWhitespaceIn(line);
	// strtol returns 0 when it fails to convert.
	currentDriver->h.releaseStringIndex = strtoul(line, &tmp, 10);
	if (line == tmp || currentDriver->h.releaseStringIndex == 0)
		{ return 0; };

	/* Spaces in the release string are expected to be escaped.
	 **/
	line = skipWhitespaceIn(tmp);
	tmp = findWhitespaceAfter(line);

	if (strlenUpToWhitespace(line, tmp) >= ZUI_DRIVER_RELEASE_MAXLEN)
		{ return 0; };

	strcpyUpToWhitespace(currentDriver->h.releaseString, line, tmp);

	if (verboseMode)
	{
		sprintf(verboseBuff, "RELEASE: %d \"%s\"",
			currentDriver->h.releaseStringIndex,
			currentDriver->h.releaseString);
	};

	return 1;
}

static int parseRequires(const char *line)
{
	char		*tmp;
	line = skipWhitespaceIn(line);
	if (currentDriver->h.nRequirements >= ZUI_DRIVER_MAX_NREQUIREMENTS)
		{ printf("%s.\n", limitExceededMessage); return 0; };

	tmp = findWhitespaceAfter(line);
	// If no whitespace, line is invalid.
	if (tmp == NULL) { return 0; };
	// Check to make sure the meta name doesn't exceed our limit.
	if (strlenUpToWhitespace(line, tmp) >= ZUI_DRIVER_REQUIREMENT_MAXLEN)
		{ return 0; };

	strcpyUpToWhitespace(
		currentDriver->requirements[currentDriver->h.nRequirements].name,
		line, tmp);

	line = skipWhitespaceIn(tmp);
	currentDriver->requirements[currentDriver->h.nRequirements].version =
		strtoul(line, &tmp, 16);

	if (line == tmp) { return 0; };

	if (verboseMode)
	{
		sprintf(verboseBuff, "REQUIRES[%d]: v%x; \"%s\"",
			currentDriver->h.nRequirements,
			currentDriver->requirements[
				currentDriver->h.nRequirements].version,
			currentDriver->requirements[
				currentDriver->h.nRequirements].name);
	};

	if (!strcmp(
		currentDriver->requirements[currentDriver->h.nRequirements].name,
		"udi"))
	{
		hasRequiresUdi = 1;
		currentDriver->h.requiredUdiVersion = currentDriver->requirements[
			currentDriver->h.nRequirements].version;

		return 1;
	};

	currentDriver->h.nRequirements++;
	return 1;
}

static int parseMeta(const char *line)
{
	char		*tmp;

	if (currentDriver->h.nMetalanguages >= ZUI_DRIVER_MAX_NMETALANGUAGES)
		{ printf("%s.\n", limitExceededMessage); return 0; };

	line = skipWhitespaceIn(line);
	currentDriver->metalanguages[currentDriver->h.nMetalanguages].index =
		strtoul(line, &tmp, 10);

	/* Meta index 0 is reserved, and strtoul() returns 0 when it can't
	 * convert. Regardless of the reason, 0 is an invalid return value for
	 * this situation.
	 **/
	if (!currentDriver->metalanguages[currentDriver->h.nMetalanguages].index)
		{ return 0; };

	line = skipWhitespaceIn(tmp);
	tmp = findWhitespaceAfter(line);

	if (strlenUpToWhitespace(line, tmp) >= ZUI_DRIVER_METALANGUAGE_MAXLEN)
		{ return 0; };

	strcpyUpToWhitespace(
		currentDriver->metalanguages[
			currentDriver->h.nMetalanguages].name,
		line, tmp);

	if (verboseMode)
	{
		sprintf(verboseBuff, "META[%d]: %d \"%s\"",
			currentDriver->h.nMetalanguages,
			currentDriver->metalanguages[
				currentDriver->h.nMetalanguages].index,
			currentDriver->metalanguages[
				currentDriver->h.nMetalanguages].name);
	};

	currentDriver->h.nMetalanguages++;
	return 1;
}

static int parseChildBops(const char *line)
{
	char		*end;

	if (currentDriver->h.nChildBops >= ZUI_DRIVER_MAX_NCHILD_BOPS)
		{ printf("%s.\n", limitExceededMessage); return 0; };

	line = skipWhitespaceIn(line);
	currentDriver->childBops[currentDriver->h.nChildBops].metaIndex =
		strtoul(line, &end, 10);

	// Regardless of the reason, 0 is an invalid return value here.
	if (!currentDriver->childBops[currentDriver->h.nChildBops].metaIndex)
		{ return 0; };

	line = skipWhitespaceIn(end);
	currentDriver->childBops[currentDriver->h.nChildBops].regionIndex =
		strtoul(line, &end, 10);

	// Region index 0 is valid.
	if (line == end) { return 0; };

	line = skipWhitespaceIn(end);
	currentDriver->childBops[currentDriver->h.nChildBops].opsIndex =
		strtoul(line, &end, 10);

	// 0 is also invalid here.
	if (!currentDriver->childBops[currentDriver->h.nChildBops].opsIndex)
		{ return 0; };

	if (verboseMode)
	{
		sprintf(verboseBuff, "CHILD_BOPS[%d]: %d %d %d",
			currentDriver->h.nChildBops,
			currentDriver->childBops[currentDriver->h.nChildBops]
				.metaIndex,
			currentDriver->childBops[currentDriver->h.nChildBops]
				.regionIndex,
			currentDriver->childBops[currentDriver->h.nChildBops]
				.opsIndex);
	};

	currentDriver->h.nChildBops++;
	return 1;
}

static int parseParentBops(const char *line)
{
	char		*end;

	if (currentDriver->h.nChildBops >= ZUI_DRIVER_MAX_NPARENT_BOPS)
		{ printf("%s.\n", limitExceededMessage); return 0; };

	line = skipWhitespaceIn(line);
	currentDriver->parentBops[currentDriver->h.nParentBops].metaIndex =
		strtoul(line, &end, 10);

	if (!currentDriver->parentBops[currentDriver->h.nParentBops].metaIndex)
		{ return 0; };

	line = skipWhitespaceIn(end);
	currentDriver->parentBops[currentDriver->h.nParentBops].regionIndex =
		strtoul(line, &end, 10);

	// 0 is a valid value for region_idx.
	if (line == end) { return 0; };

	line = skipWhitespaceIn(end);
	currentDriver->parentBops[currentDriver->h.nParentBops].opsIndex =
		strtoul(line, &end, 10);

	if (!currentDriver->parentBops[currentDriver->h.nParentBops].opsIndex)
		{ return 0; };

	line = skipWhitespaceIn(end);
	currentDriver->parentBops[currentDriver->h.nParentBops].bindCbIndex =
		strtoul(line, &end, 10);

	// 0 is a valid index value for bind_cb_idx.
	if (line == end) { return 0; };

	if (verboseMode)
	{
		sprintf(verboseBuff, "PARENT_BOPS[%d]: %d %d %d %d",
			currentDriver->h.nParentBops,
			currentDriver->parentBops[currentDriver->h.nParentBops]
				.metaIndex,
			currentDriver->parentBops[currentDriver->h.nParentBops]
				.regionIndex,
			currentDriver->parentBops[currentDriver->h.nParentBops]
				.opsIndex,
			currentDriver->parentBops[currentDriver->h.nParentBops]
				.bindCbIndex);
	};

	currentDriver->h.nParentBops++;
	return 1;
}

static int parseInternalBops(const char *line)
{
	char		*end;

	if (currentDriver->h.nChildBops >= ZUI_DRIVER_MAX_NPARENT_BOPS)
		{ printf("%s.\n", limitExceededMessage); return 0; };

	line = skipWhitespaceIn(line);
	currentDriver->internalBops[currentDriver->h.nInternalBops].metaIndex =
		strtoul(line, &end, 10);

	if (!currentDriver->internalBops[currentDriver->h.nInternalBops]
		.metaIndex)
	{
		return 0;
	};

	line = skipWhitespaceIn(end);
	currentDriver->internalBops[currentDriver->h.nInternalBops].regionIndex =
		strtoul(line, &end, 10);

	// 0 is actually not a valid value for region_idx in this case.
	if (!currentDriver->internalBops[currentDriver->h.nInternalBops]
		.regionIndex)
	{
		return 0;
	};

	line = skipWhitespaceIn(end);
	currentDriver->internalBops[currentDriver->h.nInternalBops].opsIndex0 =
		strtoul(line, &end, 10);

	if (!currentDriver->internalBops[currentDriver->h.nInternalBops]
		.opsIndex0)
	{
		return 0;
	};

	line = skipWhitespaceIn(end);
	currentDriver->internalBops[currentDriver->h.nInternalBops].opsIndex1 =
		strtoul(line, &end, 10);

	if (!currentDriver->internalBops[currentDriver->h.nInternalBops]
		.opsIndex1)
	{
		return 0;
	};

	line = skipWhitespaceIn(end);
	currentDriver->internalBops[currentDriver->h.nInternalBops].bindCbIndex =
		strtoul(line, &end, 10);

	// 0 is a valid value for bind_cb_idx.
	if (line == end) { return 0; };

	if (verboseMode)
	{
		sprintf(verboseBuff, "INTERNAL_BOPS[%d]: %d %d %d %d %d",
			currentDriver->h.nInternalBops,
			currentDriver->internalBops[
				currentDriver->h.nInternalBops].metaIndex,
			currentDriver->internalBops[
				currentDriver->h.nInternalBops].regionIndex,
			currentDriver->internalBops[
				currentDriver->h.nInternalBops].opsIndex0,
			currentDriver->internalBops[
				currentDriver->h.nInternalBops].opsIndex1,
			currentDriver->internalBops[
				currentDriver->h.nInternalBops].bindCbIndex);
	};

	currentDriver->h.nInternalBops++;
	return 1;
}

static int parseModule(const char *line)
{
	if (currentDriver->h.nModules >= ZUI_DRIVER_MAX_NMODULES)
		{ printf("%s.\n", limitExceededMessage); return 0; };

	line = skipWhitespaceIn(line);

	if (strlen(line) >= ZUI_FILENAME_MAXLEN) { return 0; };
	strcpy(currentDriver->modules[currentDriver->h.nModules].fileName, line);

	// We assign a custom module index to each module for convenience.
	currentDriver->modules[currentDriver->h.nModules].index =
		currentDriver->h.nModules;

	if (verboseMode)
	{
		sprintf(verboseBuff, "MODULE[%d]: (%d) \"%s\"",
			currentDriver->h.nModules,
			currentDriver->modules[currentDriver->h.nModules].index,
			currentDriver->modules[currentDriver->h.nModules]
				.fileName);
	};

	currentDriver->h.nModules++;
	return 1;
}

static const char *parseRegionAttribute(
	struct zui::driver::sRegion *r, const char *line, int *status
	)
{
	char			*white;

#define REGION_ATTRUTE_PARSER_PROLOGUE	\
	do { \
		white = findWhitespaceAfter(line); \
		if (white == NULL) { goto fail; }; \
		line = skipWhitespaceIn(white); \
	} while (0)


	if (!strncmp(line, "type", strlen("type")))
	{
		REGION_ATTRUTE_PARSER_PROLOGUE;

		if (!strncmp(line, "normal", strlen("normal")))
			{ goto success; };
		if (!strncmp(line, "fp", strlen("fp"))) {
			r->flags |= ZUI_REGION_FLAGS_FP; goto success;
		};
		if (!strncmp(line, "interrupt", strlen("interrupt"))) {
			r->flags |= ZUI_REGION_FLAGS_INTERRUPT; goto success;
		};

		printf("Error: Invalid value for region attribute \"type\".\n");
		goto fail;
	};

	if (!strncmp(line, "binding", strlen("binding")))
	{
		REGION_ATTRUTE_PARSER_PROLOGUE;

		if (!strncmp(line, "static", strlen("static")))
			{ goto success; };
		if (!strncmp(line, "dynamic", strlen("dynamic"))) {
			r->flags |= ZUI_REGION_FLAGS_DYNAMIC; goto success;
		};

		printf("Error: Invalid value for region attribute "
			"\"binding\".\n");

		goto fail;
	};

	if (!strncmp(line, "priority", strlen("priority")))
	{
		REGION_ATTRUTE_PARSER_PROLOGUE;

		if (!strncmp(line, "lo", strlen("lo"))) {
			r->priority = zui::driver::REGION_PRIO_LOW; goto success;
		};
		if (!strncmp(line, "med", strlen("med"))) {
			r->priority = zui::driver::REGION_PRIO_MEDIUM; goto success;
		};
		if (!strncmp(line, "hi", strlen("hi"))) {
			r->priority = zui::driver::REGION_PRIO_HIGH; goto success;
		};

		printf("Error: Invalid value for region attribute "
			"\"priority\".\n");

		goto fail;
	};

	if (!strncmp(line, "latency", strlen("latency"))
		|| !strncmp(line, "overrun_time", strlen("overrun_time")))
	{
		REGION_ATTRUTE_PARSER_PROLOGUE;

		fprintf(
			stderr,
			"Warning: \"latency\" and \"overrun_time\" "
			"region attributes are currently silently "
			"ignored.\n");

		goto success;
	};

	printf("Error: Unknown region attribute.\n");
	goto fail;

success:
	white = findWhitespaceAfter(line);
	*status = 1;
	return line + strlenUpToWhitespace(line, white);

fail:
	*status = 0;
	return NULL;
}

static void *parseRegion(const char *line)
{
	struct zui::driver::sRegion	*ret;
	char				*tmp;
	int				status;

	PARSER_MALLOC(&ret, struct zui::driver::sRegion);
	line = skipWhitespaceIn(line);

	ret->driverId = currentDriver->h.id;
	if (currentDriver->h.nModules == 0)
	{
		printf("Error: a module statement must precede any regions.\n"
			"Regions must belong to a module.\n");

		goto releaseAndExit;
	};

	ret->moduleIndex = currentDriver->h.nModules - 1;
	ret->index = strtoul(line, &tmp, 10);
	if (line == tmp) { goto releaseAndExit; };

	line = skipWhitespaceIn(tmp);
	// If there are no attributes following the index, skip attrib parsing.

	if (*line != '\0')
	{
		do
		{
			line = parseRegionAttribute(ret, line, &status);
			if (status == 0) { goto releaseAndExit; };

			line = skipWhitespaceIn(line);
		} while (*line != '\0');
	};

	if (verboseMode)
	{
		sprintf(verboseBuff, "REGION(%d): (module %d \"%s\"): "
			"Prio: %d, latency %d, dyn.? %c, FP? %c, intr.? %c",
			ret->index,
			ret->moduleIndex,
			currentDriver->modules[ret->moduleIndex].fileName,
			(int)ret->priority, (int)ret->latency,
			(ret->flags & ZUI_REGION_FLAGS_DYNAMIC) ? 'y' : 'n',
			(ret->flags & ZUI_REGION_FLAGS_FP) ? 'y' : 'n',
			(ret->flags & ZUI_REGION_FLAGS_INTERRUPT) ? 'y' : 'n');
	};

	currentDriver->h.nRegions++;
	return ret;
PARSER_RELEASE_AND_EXIT(&ret);
}

static uint8_t getDigit(const char c)
{
	if (c < '0'
		|| (c > '9' && c < 'A')
		|| (c > 'F' && c < 'a')
		|| c > 'f')
	{
		return 0xFF;
	};

	if (c <= '9') { return c - '0'; };
	if (c <= 'F') { return c - 'A' + 10; };
	return c - 'a' + 10;
}

static const char *parseDeviceAttribute(
	struct zui::device::_sDevice *d, const char *line, int *status
	)
{
	char		*white;
	int		retOffset, i, j;
	uint8_t		byte;

	// Get the attribute name.
	white = findWhitespaceAfter(line);
	if (white == NULL) { goto fail; };

	if (strlenUpToWhitespace(line, white) >= UDI_MAX_ATTR_NAMELEN)
		{ goto fail; };

	strcpyUpToWhitespace(
		d->d[d->h.nAttributes].attr_name,
		line, white);

	// Get the attribute type.
	line = skipWhitespaceIn(white);
	white = findWhitespaceAfter(line);
	if (white == NULL) { goto fail; };

	if (!strncmp(line, "string", strlen("string"))) {
		d->d[d->h.nAttributes].attr_type =
			UDI_ATTR_STRING;

		line = skipWhitespaceIn(white);

		white = findWhitespaceAfter(line);
		retOffset = strlenUpToWhitespace(line, white);
		if (retOffset >= UDI_MAX_ATTR_SIZE)
			{ goto fail; };

		strcpyUpToWhitespace(
			(char *)d->d[d->h.nAttributes].attr_value,
			line, white);

		goto success;
	};

	if (!strncmp(line, "ubit32", strlen("ubit32"))) {
		d->d[d->h.nAttributes].attr_type =
			UDI_ATTR_UBIT32;

		line = skipWhitespaceIn(white);
//		d->d[d->h.nAttributes].value.u32val =
//			strtoul(line, &white, 0);
		UDI_ATTR32_SET(
			d->d[d->h.nAttributes].attr_value,
			strtoul(line, &white, 0));

		if (line == white) { goto fail; };

		white = findWhitespaceAfter(line);
		retOffset = strlenUpToWhitespace(line, white);
		goto success;
	};

	if (!strncmp(line, "boolean", strlen("boolean"))) {
		d->d[d->h.nAttributes].attr_type = UDI_ATTR_BOOLEAN;
		line = skipWhitespaceIn(white);
		if (*line == 't' || *line == 'T') {
			d->d[d->h.nAttributes].attr_value[0] = 1;
		} else if (*line == 'f' || *line == 'F') {
			d->d[d->h.nAttributes].attr_value[0] = 0;
		} else { goto fail; };

		retOffset = 1;
		goto success;
	};

	if (!strncmp(line, "array", strlen("array"))) {
		d->d[d->h.nAttributes].attr_type =
			UDI_ATTR_ARRAY8;

		line = skipWhitespaceIn(white);

		white = findWhitespaceAfter(line);
		retOffset = strlenUpToWhitespace(line, white);
		// The ARRAY8 values come in pairs; the strlen cannot be odd.
		if (retOffset > UDI_MAX_ATTR_SIZE
			|| (retOffset % 2) != 0)
			{ goto fail; };

		for (i=0, j=0; i<retOffset; i++)
		{
			byte = getDigit(line[i]);
			if (byte > 15) { goto fail; };
			if (i % 2 == 0) {
				d->d[d->h.nAttributes].attr_value[j] = byte << 4;
			}
			else
			{
				d->d[d->h.nAttributes].attr_value[j] |= byte;
				j++;
			};
		};

		d->d[d->h.nAttributes].attr_length = j;
		goto success;
	};
fail:
	*status = 0;
	return NULL;

success:
	*status = 1;
	return line + retOffset;
}

static void *parseDevice(const char *line)
{
	struct zui::device::_sDevice	*ret;
	char				*tmp;
	int				status, i, j, printLen;

	PARSER_MALLOC(&ret, struct zui::device::_sDevice);
	ret->h.index = currentDriver->h.nDevices;
	line = skipWhitespaceIn(line);
	ret->h.messageIndex = strtoul(line, &tmp, 10);
	// 0 is invalid regardless of the reason.
	if (ret->h.messageIndex == 0) { goto releaseAndExit; };
	line = skipWhitespaceIn(tmp);
	ret->h.metaIndex = strtoul(line, &tmp, 10);
	if (ret->h.metaIndex == 0) { goto releaseAndExit; };
	line = skipWhitespaceIn(tmp);
	// This is where we loop, trying to parse for attributes.
	if (*line != '\0')
	{
		do
		{
			line = parseDeviceAttribute(ret, line, &status);
			if (status == 0) { goto releaseAndExit; };
			ret->h.nAttributes++;
			line = skipWhitespaceIn(line);
		} while (*line != '\0');
	};

	if (verboseMode)
	{
		printLen = sprintf(
			verboseBuff,
			"DEVICE(index %d, %d, %d, %d attrs)",
			ret->h.index, ret->h.messageIndex, ret->h.metaIndex,
			ret->h.nAttributes);

		for (i=0; i<ret->h.nAttributes; i++)
		{
			printLen += sprintf(&verboseBuff[printLen], ".\n");
			switch (ret->d[i].attr_type)
			{
			case UDI_ATTR_STRING:
				printLen += sprintf(
					&verboseBuff[printLen],
					"\tSTR %s: \"%s\"",
					ret->d[i].attr_name,
					ret->d[i].attr_value);

				break;
			case UDI_ATTR_ARRAY8:
				printLen += sprintf(
					&verboseBuff[printLen],
					"\tARR %s: size %d: ",
					ret->d[i].attr_name,
					ret->d[i].attr_length);

				for (j=0; j<ret->d[i].attr_length; j++)
				{
					printLen += sprintf(
						&verboseBuff[printLen],
						"%02X",
						ret->d[i].attr_value[j]);
				};

				break;
			case UDI_ATTR_BOOLEAN:
				printLen += sprintf(
					&verboseBuff[printLen],
					"\tBOOL %s: %d",
					ret->d[i].attr_name,
					ret->d[i].attr_value[0]);

				break;
			case UDI_ATTR_UBIT32:
				printLen += sprintf(
					&verboseBuff[printLen],
					"\tU32 %s: 0x%x",
					ret->d[i].attr_name,
					UDI_ATTR32_GET(ret->d[i].attr_value));

				break;
			};
		};
	};

	ret->h.driverId = currentDriver->h.id;
	currentDriver->h.nDevices++;
	return ret;
PARSER_RELEASE_AND_EXIT(&ret);
}

static void *parseProvides(const char *line)
{
	struct zui::driver::_sProvision	*ret;
	char				*tmp;

	PARSER_MALLOC(&ret, struct zui::driver::_sProvision);

	line = skipWhitespaceIn(line);
	tmp = findWhitespaceAfter(line);
	if (tmp == NULL) { goto releaseAndExit; };
	if (strlenUpToWhitespace(line, tmp) >= ZUI_PROVISION_NAME_MAXLEN)
		{ goto releaseAndExit; };

	strcpyUpToWhitespace(ret->name, line, tmp);

	// Now get the release version.
	line = skipWhitespaceIn(tmp);
	ret->version = strtoul(line, &tmp, 0);
	if (line == tmp) { goto releaseAndExit; };

	ret->driverId = currentDriver->h.id;

	if (verboseMode)
	{
		sprintf(verboseBuff, "PROVIDES (v0x%x): \"%s\"",
			ret->version, ret->name);
	};

	currentDriver->h.nProvisions++;
	return ret;
PARSER_RELEASE_AND_EXIT(&ret);
}

static int parseRankAttribute(struct zui::rank::_sRank *rank, const char *line)
{
	char		*white;

	white = findWhitespaceAfter(line);
	if (strlenUpToWhitespace(line, white) >= UDI_MAX_ATTR_NAMELEN)
		{ return 0; };

	strcpyUpToWhitespace(rank->d[rank->h.nAttributes].name, line, white);
	return 1;
}

static void *parseRank(const char *line)
{
	struct zui::rank::_sRank		*ret;
	char				*tmp;
	int				status, i, printLen=0;

	PARSER_MALLOC(&ret, struct zui::rank::_sRank);
	ret->h.driverId = currentDriver->h.id;

	line = skipWhitespaceIn(line);

	ret->h.rank = strtoul(line, &tmp, 10);
	// AFAICT, rank 0 is reserved.
	if (ret->h.rank == 0) { goto releaseAndExit; };

	tmp = findWhitespaceAfter(line);
	// At least one attribute is required.
	if (tmp == NULL || *tmp == '\0') { goto releaseAndExit; };
	line = skipWhitespaceIn(tmp);

	do
	{
		if (ret->h.nAttributes >= ZUI_RANK_MAX_NATTRS)
		{
			fprintf(stderr, "%s.\n", limitExceededMessage);
			goto releaseAndExit;
		};

		status = parseRankAttribute(ret, line);
		if (!status) { goto releaseAndExit; };
		ret->h.nAttributes++;
		line = findWhitespaceAfter(line);
		if (line == NULL) { continue; };
		line = skipWhitespaceIn(line);
	} while (line != NULL && *line != '\0');

	currentDriver->h.nRanks++;

	if (verboseMode)
	{
		printLen += sprintf(
			verboseBuff, "RANK %d: %d attribs",
			ret->h.rank, ret->h.nAttributes);

		for (i=0; i<ret->h.nAttributes; i++)
		{
			printLen += sprintf(
				&verboseBuff[printLen], ".\n\tAttr: \"%s\"",
				ret->d[i].name);
		};
	};

	return ret;
PARSER_RELEASE_AND_EXIT(&ret);
}

static int parseCategory(const char *line)
{
	char		*tmp;

	line = skipWhitespaceIn(line);

	currentDriver->h.categoryIndex = strtoul(line, &tmp, 10);
	if (currentDriver->h.categoryIndex == 0) { return 0; };

	if (verboseMode)
	{
		sprintf(verboseBuff, "CATEGORY: %d",
			currentDriver->h.categoryIndex);
	};

	return 1;
}

enum parser_lineTypeE parser_parseLine(const char *line, void **ret)
{
	int			slen;

	if (currentDriver == NULL) { return LT_UNKNOWN; };
	line = skipWhitespaceIn(line);
	// Skip lines with only whitespace.
	if (line[0] == '\0') { return LT_MISC; };

	/* message_file must come before message or else message will match all
	 * message_file lines as well.
	 **/
	if (!strncmp(line, "message_file", slen = strlen("message_file"))) {
		*ret = parseMessageFile(&line[slen]);
		return (*ret == NULL) ? LT_INVALID : LT_MESSAGE_FILE;
	};

	if (!strncmp(line, "message", slen = strlen("message"))) {
		*ret = parseMessage(&line[slen]);
		return (*ret == NULL) ? LT_INVALID : LT_MESSAGE;
	};

	if (!strncmp(line, "requires", slen = strlen("requires"))) {
		return (parseRequires(&line[slen])) ? LT_DRIVER : LT_INVALID;
	};

	if (!strncmp(
		line, "pio_serialization_limit",
		slen = strlen("pio_serialization_limit")))
		{ return LT_MISC; };

	if (!strncmp(
		line, "compile_options", slen = strlen("compile_options")))
		{ return LT_MISC; };

	if (!strncmp(
		line, "source_files", slen = strlen("source_files")))
		{ return LT_MISC; };

	if (!strncmp(
		line, "source_requires", slen = strlen("source_requires")))
		{ return LT_MISC; };

	if (!strncmp(line, "module", slen = strlen("module"))) {
		return (parseModule(&line[slen])) ? LT_DRIVER : LT_INVALID;
	};

	if (!strncmp(
		line, "disaster_message", slen = strlen("disaster_message"))) {
		*ret = parseDisasterMessage(&line[slen]);
		return (*ret == NULL) ? LT_INVALID : LT_DISASTER_MESSAGE;
	};

	if (!strncmp(line, "locale", slen = strlen("locale")))
		{ return LT_MISC; };

	if (!strncmp(line, "release", slen = strlen("release")))
		{ return (parseRelease(&line[slen])) ? LT_DRIVER:LT_INVALID; };

	if (!strncmp(line, "name", slen = strlen("name")))
		{ return (parseName(&line[slen])) ? LT_DRIVER : LT_INVALID; };

	if (!strncmp(line, "contact", slen = strlen("contact")))
		{ return (parseContact(&line[slen])) ? LT_DRIVER:LT_INVALID; };

	if (!strncmp(
		line, "properties_version",
		slen = strlen("properties_version")))
		{ return LT_MISC; };

	if (!strncmp(line, "supplier", slen = strlen("supplier")))
		{ return (parseSupplier(&line[slen])) ? LT_DRIVER:LT_INVALID; };

	if (!strncmp(line, "shortname", slen = strlen("shortname"))) {
		return (parseShortName(&line[slen])) ? LT_DRIVER : LT_INVALID;
	};

	if (!strncmp(line, "source_requires", slen = strlen("source_requires")))
		{ return LT_MISC; };

	if (propsType == DRIVER_PROPS)
	{
		if (!strncmp(line, "multi_parent", slen = strlen("multi_parent")))
			{ return LT_MISC; };

		if (!strncmp(line, "enumerates", slen = strlen("enumerates")))
			{ return LT_MISC; };

		if (!strncmp(
			line, "internal_bind_ops", slen = strlen("internal_bind_ops")))
		{
			return (parseInternalBops(&line[slen]))
				? LT_INTERNAL_BOPS : LT_INVALID;
		};

		if (!strncmp(line, "parent_bind_ops", slen=strlen("parent_bind_ops"))) {
			return (parseParentBops(&line[slen]))
				? LT_PARENT_BOPS : LT_INVALID;
		};

		if (!strncmp(line, "child_bind_ops", slen = strlen("child_bind_ops"))) {
			return (parseChildBops(&line[slen]))
				? LT_CHILD_BOPS : LT_INVALID;
		};

		if (!strncmp(line, "meta", slen = strlen("meta"))) {
			return (parseMeta(&line[slen])) ? LT_METALANGUAGE : LT_INVALID;
		};

		if (!strncmp(line, "device", slen = strlen("device"))) {
			*ret = parseDevice(&line[slen]);
			return (*ret == NULL) ? LT_INVALID : LT_DEVICE;
		};

		if (!strncmp(line, "region", slen = strlen("region"))) {
			*ret = parseRegion(&line[slen]);
			return (*ret == NULL) ? LT_INVALID : LT_REGION;
		};

		if (!strncmp(line, "readable_file", slen = strlen("readable_file"))) {
			*ret = parseReadableFile(&line[slen]);
			return (*ret == NULL) ? LT_INVALID : LT_DISASTER_MESSAGE;
		};

		if (!strncmp(line, "custom", slen = strlen("custom")))
			{ return LT_MISC; };

		if (!strncmp(line, "config_choices", slen = strlen("config_choices")))
			{ return LT_MISC; };
	};

	if (propsType == META_PROPS)
	{
		if (!strncmp(line, "provides", slen = strlen("provides"))) {
			*ret = parseProvides(&line[slen]);
			return (*ret == NULL) ? LT_INVALID : LT_PROVIDES;
		};

		if (!strncmp(line, "symbols", slen = strlen("symbols")))
			{ return LT_MISC; };

		if (!strncmp(line, "category", slen = strlen("category"))) {
			return (parseCategory(&line[slen]))
				? LT_DRIVER : LT_INVALID;
		};

		// Does not seem like rank is supported by the spec anymore.
		if (!strncmp(line, "rank", slen = strlen("rank"))) {
			*ret = parseRank(&line[slen]);
			return (*ret == NULL) ? LT_INVALID : LT_RANK;
		};
	};

	return LT_UNKNOWN;
}

