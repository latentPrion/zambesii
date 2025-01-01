
#include <limits.h>
#include <cstring>
#include <stdint.h>
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <sys/stat.h>
#include "zudipropsc.h"


/**	EXPLANATION:
 * Simple udiprops parser that creates an index of UDI driver static properties
 * in a format that can be parsed by the Zambesii kernel.
 *
 * Has two modes of operation, determined by the first argument:
 *
 * Mode 1: "Kernel-index":
 *	This mode is meant to be used when building the Zambesii kernel itself.
 *	It will take the name of an input file (second argument) which contains
 *	a newline separated list of udiprops files.
 *
 *	It will then parse each of these udiprops files, building an index of
 *	unified driver properties which it will then dump in a format
 *	appropriate for the Zambesii kernel. The output files will constitute
 *	the in-kernel-driver index.
 *
 * Mode 2: "User-index":
 *	This mode of operation is meant to be used when building the userspace
 *	driver index, or the kernel's ram-disk's index. It will take an input
 *	file (second argument) which contains a newline-separated list of
 *	already-compiled binary UDI drivers.
 *
 *	Each of these drivers will be opened, and their .udiprops section read,
 *	and the index will be built from these binary UDI drivers.
 **/
#define UDIPROPS_LINE_MAXLEN		(512)

static const char *usageMessage = "Usage:\n\tzudiindex -<c|a|l|r> "
					"<file|endianness> "
					"[-txt|-bin] "
					" [-i <index-dir>] [-b <base-path>]\n"
					"Note: For --printsizes, include one "
					"or more dummy arguments";

enum parseModeE		parseMode=PARSE_NONE;
enum programModeE	programMode=MODE_NONE;
enum propsTypeE		propsType=DRIVER_PROPS;
int			hasRequiresUdi=0, hasRequiresUdiPhysio=0, verboseMode=0,
			ignoreInvalidBasePath=0;

const char		*indexPath=NULL, *basePath=NULL, *inputFileName=NULL;
char			propsLineBuffMem[515];
char			verboseBuff[1024];

static void parseCommandLine(int argc, char **argv)
{
	int		i, actionArgIndex,
			basePathArgIndex=-1, indexPathArgIndex=-1;

	if (argc < 3) { exit(printAndReturn(argv[0], usageMessage, 1)); };

	// Check for verbose switch.
	for (i=1; i<argc; i++)
	{
		if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--verbose"))
			{ verboseMode = 1; continue; };

		if (!strcmp(argv[i], "-meta"))
			{ propsType = META_PROPS; continue; };

		if (!strcmp(argv[i], "--ignore-invalid-basepath"))
			{ ignoreInvalidBasePath = 1; continue; };
	};

	// First find out the action we are to carry out.
	for (i=1; i<argc; i++)
	{
		// No further comm. line processing necessary here.
		if (!strcmp(argv[i], "--printsizes"))
			{ programMode = MODE_PRINT_SIZES; return; };

		if (!strcmp(argv[i], "-a"))
			{ programMode = MODE_ADD; break; };

		if (!strcmp(argv[i], "-l"))
			{ programMode = MODE_LIST; break; };

		if (!strcmp(argv[i], "-r"))
			{ programMode = MODE_REMOVE; break; };

		if (!strcmp(argv[i], "-c"))
			{ programMode = MODE_CREATE; break; };
	};

	actionArgIndex = i;

	/* Now get the parse mode. Only matters in ADD mode.
	 **/
	if (programMode == MODE_ADD)
	{
		for (i=0; i<argc; i++)
		{
			if (!strcmp(argv[i], "-txt"))
				{ parseMode = PARSE_TEXT; break; };

			if (!strcmp(argv[i], "-bin"))
				{ parseMode = PARSE_BINARY; break; };
		};

		// If the parse mode wasn't specified:
		if (parseMode == PARSE_NONE)
		{
			exit(
				printAndReturn(
					argv[0],
					"Input file format [-txt|bin] must be "
					"included when adding new drivers",
					EX_BAD_COMMAND_LINE));
		};
	};

	/* If no mode was detected or if the index of the action switch was
	 * invalid (that is, it overflows "argc"), we exit the program.
	 **/
	if (programMode == MODE_NONE || actionArgIndex + 1 >= argc)
	{
		exit(
			printAndReturn(
				argv[0], usageMessage, EX_BAD_COMMAND_LINE));
	};

	inputFileName = argv[actionArgIndex + 1];

	// In list mode, no more than 5 arguments are valid.
	if (programMode == MODE_LIST)
	{
		if (argc > 5)
		{
			exit(
				printAndReturn(
					argv[0], usageMessage,
					EX_BAD_COMMAND_LINE));
		};
	};

	for (i=1; i<argc; i++)
	{
		if (!strcmp(argv[i], "-i") || !strcmp(argv[i], "--indexpath"))
			{ indexPathArgIndex = i; continue; };

		if (!strcmp(argv[i], "-b") || !strcmp(argv[i], "--basepath"))
			{ basePathArgIndex = i; continue; };
	};

	/* Ensure that the index for the index-path or base-path arguments isn't
	 * beyond the bounds of the number of arguments we actually got.
	 **/
	if (indexPathArgIndex + 1 >= argc || basePathArgIndex + 1 >= argc) {
		exit(printAndReturn(argv[0], usageMessage, EX_BAD_COMMAND_LINE));
	};

	// If no index path was explicitly provided, assume a default path.
	if (indexPathArgIndex == -1) { indexPath = "@h:zambesii/drivers"; }
	else { indexPath = argv[indexPathArgIndex + 1]; };

	// CREATE mode only needs the endianness and the index path.
	if (programMode == MODE_CREATE || programMode == MODE_LIST) { return; };

	if (basePathArgIndex == -1 && programMode == MODE_ADD)
	{
		exit(
			printAndReturn(
				argv[0],
				"Base path must be provided when adding new "
				"drivers",
				EX_BAD_COMMAND_LINE));
	};

	if (basePathArgIndex != -1) { basePath = argv[basePathArgIndex + 1]; };
	// basepath is required in ADD and accepted in REMOVE.
	if (basePath != NULL && strlen(basePath) >= ZUI_DRIVER_BASEPATH_MAXLEN)
	{
		std::cout <<"This program accepts basepaths with up to "
			<<ZUI_DRIVER_BASEPATH_MAXLEN
			<<" characters.\n";

		exit(
			printAndReturn(
				argv[0], "Base path exceeds max accepted "
				"characters", EX_BAD_COMMAND_LINE));
	};
}

char *makeFullName(
	char *reallocMem, const char *indexPath, const char *fileName
	)
{
	char		*ret;
	int		pathLen;

	pathLen = strlen(indexPath);
	ret = (char *)realloc(reallocMem, pathLen + strlen(fileName) + 2);
	if (ret == NULL) { return NULL; };

	strcpy(ret, indexPath);
	if (indexPath[pathLen - 1] != '/') { strcat(ret, "/"); };
	strcat(ret, fileName);
	return ret;
}

const char		*indexFileNames[] =
{
	"drivers.zudi-index", "data.zudi-index",
	"devices.zudi-index", "strings.zudi-index",
	"ranks.zudi-index", "provisions.zudi-index",
	// Terminate this list with a NULL always.
	NULL
};

static int createMode(int argc, char **argv)
{
	FILE				*currFile;
	char				*fullName=NULL;
	int				i, blocksWritten;
	struct zui::sHeader		*indexHeader;
	(void)argc; (void)argv;

	/**	EXPLANATION:
	 * Creates a new series of index files. Clears and overwrites any index
	 * files already in existence in the index path.
	 **/
	if (strcmp(inputFileName, "le") && strcmp(inputFileName, "be"))
	{
		std::cerr <<"Error: CREATE mode requires endianness.\n"
			"\t-c <le|be>.\n";

		return EX_BAD_COMMAND_LINE;
	};

	// Allocate and fill in the index header.
	indexHeader = (zui::sHeader *)malloc(sizeof(*indexHeader));
	if (indexHeader == NULL) { return EX_NOMEM; };
	memset(indexHeader, 0, sizeof(*indexHeader));

	// The rest of the fields can remain blank for now.
	strcpy(indexHeader->endianness, inputFileName);

	for (i=0; indexFileNames[i] != NULL; i++)
	{
		fullName = makeFullName(fullName, indexPath, indexFileNames[i]);
		currFile = fopen(fullName, "w");
		if (currFile == NULL)
		{
			std::cerr <<"Error: Failed to create index file "
				<<fullName <<" .\n";

			return EX_FILE_OPEN;
		};

		if (strcmp(indexFileNames[i], "drivers.zudi-index") != 0)
		{
			fclose(currFile);
			continue;
		};

		blocksWritten = fwrite(
			indexHeader, sizeof(*indexHeader), 1, currFile);

		if (blocksWritten < 1)
		{
			std::cerr <<"Error: Failed to write index header "
				"to driver index file.\n";

			return EX_FILE_IO;
		};

		fclose(currFile);
	};

	return EXIT_SUCCESS;
}

static int binaryParse(FILE *propsFile, char *propsLineBuff)
{
	(void) propsFile;
	(void) propsLineBuff;
	/**	EXPLANATION:
	 * In user-index mode, the filenames in the list file are all compiled
	 * UDI driver binaries. They contain udiprops within their .udiprops
	 * executable section. This function will be called once for each driver
	 * in the list.
	 *
	 * Open the driver file, parse the ELF headers to find the offset and
	 * length of the .udiprops section, extract it from the driver file
	 * and then parse it into the index before returning.
	 **/
	return EXIT_SUCCESS;
}

const char		*lineTypeStrings[] =
{
	"UNKNOWN", "INVALID", "OVERFLOW", "LIMIT_EXCEEDED", "MISC",
	"DRIVER", "MODULE", "REGION", "DEVICE",
	"MESSAGE", "DISASTER_MESSAGE",
	"MESSAGE_FILE",
	"CHILD_BIND_OPS", "INTERNAL_BIND_OPS", "PARENT_BIND_OPS",
	"METALANGUAGE", "READABLE_FILE", "LT_RANK", "LT_PROVIDES"
};

static inline int isBadLineType(enum parser_lineTypeE lineType)
{
	return (lineType == LT_UNKNOWN || lineType == LT_INVALID
		|| lineType == LT_OVERFLOW || lineType == LT_LIMIT_EXCEEDED);
}

static void printBadLineType(enum parser_lineTypeE lineType, int logicalLineNo)
{
	if (lineType == LT_UNKNOWN)
	{
		std::cerr <<"Line "
			<<logicalLineNo
			<<": Error: Unknown statement. Aborting.\n";
	};

	if (lineType == LT_INVALID)
	{
		std::cerr <<"Line "
			<<logicalLineNo
			<<": Error: Invalid arguments to statement. Aborting.\n";
	};

	if (lineType == LT_OVERFLOW)
	{
		std::cerr <<"Line "
			<<logicalLineNo
			<<": Error: an argument overflows either "
			"a UDI imposed limit, or a limit imposed by this "
			"program.\n";
	};

	if (lineType == LT_LIMIT_EXCEEDED)
	{
		std::cerr <<"Line "
			<<logicalLineNo
			<<": Error: The number of statements of "
			"this type exceeds the capability of this program to "
			"process. This is not a UDI-defined limit.\n";
	};
}

static void verboseModePrint(
	enum parser_lineTypeE lineType, int logicalLineNo,
	const char *verboseString, const char *rawLineString
	)
{
	if (lineType == LT_MISC)
	{
		std::cout <<"Line "
			<<std::setfill('0') << std::setw(3) << logicalLineNo
			<<"(" <<lineTypeStrings[lineType] <<"): "
			<<"\"" <<rawLineString <<"\".\n";

		return;
	};

	if (!isBadLineType(lineType))
	{
		std::cout <<"Line "
			<<std::setfill('0') <<std::setw(3) <<logicalLineNo <<": "
			<<verboseString <<".\n";
	};

}

static int textParse(FILE *propsFile, char *propsLineBuff)
{
	int		logicalLineNo, lineSegmentLength, buffIndex,
			isMultiline, lineLength;
	char		*comment;
	enum parser_lineTypeE	lineType=LT_MISC;
	void			*indexObj, *ptr;
	(void) ptr;

	/**	EXPLANATION:
	 * In kernel-index mode, the filenames in the list file are all directly
	 * udiprops files. This function will be called once for each file in
	 * the list. Parse the data in the file, add it to the index, and
	 * return.
	 **/
	/* Now loop, getting lines and pass them to the parser. Since the parser
	 * expects only fully stripped lines, we strip lines in here before
	 * passing them to the parser.
	 **/
	logicalLineNo = 0;
	do
	{
		/* Read one "line", where "line" may be a line that spans
		 * newline-separated line-segments, indicated by the '\'
		 * character.
		 **/
		buffIndex = 0;
		lineLength = 0;
		logicalLineNo++;

		do
		{
			ptr = fgets(
				&propsLineBuff[buffIndex], UDIPROPS_LINE_MAXLEN,
				propsFile);

			if (feof(propsFile)) { break; };

			// Check for (and omit) comments (prefixed with #).
			comment = strchr(&propsLineBuff[buffIndex], '#');
			if (comment != NULL)
			{
				lineSegmentLength =
					comment - &propsLineBuff[buffIndex];
			}
			else
			{
				lineSegmentLength = strlen(
					&propsLineBuff[buffIndex]);

				// Consume the trailing EOL (including \r).
				lineSegmentLength--;
				if (propsLineBuff[
					buffIndex+lineSegmentLength-1] == '\r')
				{
					lineSegmentLength--;
				};
			};

			// TODO: Check for lines that are too long.
			propsLineBuff[buffIndex + lineSegmentLength] = '\0';
			if (propsLineBuff[buffIndex + lineSegmentLength-1]
				== '\\')
			{
				isMultiline = 1;
				buffIndex += lineSegmentLength - 1;
				lineLength += lineSegmentLength - 1;
			}
			else
			{
				isMultiline = 0;
				lineLength += lineSegmentLength;
			};
		}
		while (isMultiline);

		// Don't waste time calling the parser on 0 length lines.
		if (lineLength < 2) { continue; };
		lineType = parser_parseLine(propsLineBuff, &indexObj);

		if (verboseMode)
		{
			verboseModePrint(
				lineType, logicalLineNo, verboseBuff,
				propsLineBuff);
		};

		if (isBadLineType(lineType))
		{
			printBadLineType(lineType, logicalLineNo);
			break;
		};

		if (index_insert(lineType, indexObj) != EX_SUCCESS) { break; };
	} while (!feof(propsFile));

	return (feof(propsFile)) ? EX_SUCCESS : EX_PARSE_ERROR;
}

int incrementNRecords(uint32_t nSupportedDevices, uint32_t nSupportedMetas)
{
	FILE				*dhFile;
	struct zui::sHeader		*header;
	char				*fullName=NULL;

	fullName = makeFullName(
		fullName, indexPath, "drivers.zudi-index");

	if (fullName == NULL) { return EX_NOMEM; };

	header = new zui::sHeader;
	if (header == NULL) { return EX_NOMEM; };

	dhFile = fopen(fullName, "r+");
	if (dhFile == NULL) { return EX_FILE_OPEN; };

	if (fread(header, sizeof(*header), 1, dhFile) < 1)
	{
		std::cerr <<"Error: Failed to read index header for "
			"nRecords update.\n";

		return EX_FILE_IO;
	};

	header->nRecords++;
	header->nSupportedDevices += nSupportedDevices;
	header->nSupportedMetas += nSupportedMetas;

	if (fseek(dhFile, 0, SEEK_SET) != 0)
		{ fclose(dhFile); return EX_FILE_IO; };

	if (fwrite(header, sizeof(*header), 1, dhFile) < 1)
	{
		std::cerr <<"Error: Failed to rewrite index header after "
			"nRecords update.\n";

		return EX_FILE_IO;
	};

	fclose(dhFile);
	return EX_SUCCESS;
}

static int getNextDriverId(uint32_t *driverId)
{
	struct zui::sHeader		*driverHeader;
	FILE				*driverHeaderIndex;
	char				*fullName=NULL;

	fullName = makeFullName(
		fullName, indexPath, "drivers.zudi-index");

	if (fullName == NULL) { return 0; };

	driverHeader = new zui::sHeader;
	if (driverHeader == NULL) { return 0; };

	driverHeaderIndex = fopen(fullName, "r+");
	if (driverHeaderIndex == NULL)
	{
		std::cerr <<"Error: Failed to open driver-header index.\n";
		return 0;
	};

	if (fread(driverHeader, sizeof(*driverHeader), 1, driverHeaderIndex)
		< 1)
	{
		fclose(driverHeaderIndex);
		std::cerr <<"Error: Failed to read index header.\n";
		return 0;
	};

	*driverId = driverHeader->nextDriverId++;

	if (fseek(driverHeaderIndex, 0, SEEK_SET) != 0)
		{ fclose(driverHeaderIndex); return 0; };

	if (fwrite(driverHeader, sizeof(*driverHeader), 1, driverHeaderIndex)
		< 1)
	{
		fclose(driverHeaderIndex);
		std::cerr <<"Error: Failed to rewrite index header.\n";
		return 0;
	};

	fclose(driverHeaderIndex);
	return 1;
}

static int addMode(int argc, char **argv)
{
	FILE		*iFile;
	int		ret;
	(void)		argc;
	uint32_t	driverId;
	int		nSupportedDevices, nSupportedMetas;

	// Try to open up the input file.
	iFile = fopen(
		inputFileName,
		((parseMode == PARSE_TEXT) ? "r" : "rb"));

	if (iFile == NULL)
	{
		exit(
			printAndReturn(
				argv[0], "Invalid input file",
				EX_INVALID_INPUT_FILE));
	};

	if (!getNextDriverId(&driverId))
	{
		exit(printAndReturn(
			argv[0], "Failed to get next driver ID",
			EX_UNKNOWN));
	};

	parser_initializeNewDriverState(driverId);
	index_initialize();
	if (parseMode == PARSE_TEXT) {
		ret = textParse(iFile, propsLineBuffMem);
	} else {
		ret = binaryParse(iFile, propsLineBuffMem);
	};

	if (ret != EX_SUCCESS)
	{
		exit(printAndReturn(
			argv[0], "Error: Parse stage returned error",
			ret));
	};

	// Some extra checks.
	if (!hasRequiresUdi)
	{
		exit(
			printAndReturn(
				argv[0], "Error: Driver does not have requires "
				"udi.\n",
				EX_NO_REQUIRES_UDI));
	};

	ret = index_writeToDisk();
	if (ret != EX_SUCCESS)
	{
		exit(printAndReturn(
			argv[0], "Error: Failed to write index to disk files",
			ret));
	};

	index_free();
	nSupportedDevices = parser_getNSupportedDevices();
	nSupportedMetas = parser_getNSupportedMetas();
	parser_releaseState();
	fclose(iFile);
	return incrementNRecords(nSupportedDevices, nSupportedMetas);
}

static struct stat		dirStat;

int fileExists(const char *path)
{
	if (stat(path, &dirStat) != 0) { return 0; };
	if (!S_ISREG(dirStat.st_mode)) { return 0; };
	return 1;
}

int folderExists(const char *path)
{
	if (stat(path, &dirStat) != 0) { return 0; };
	if (!S_ISDIR(dirStat.st_mode)) { return 0; };
	return 1;
}

int main(int argc, char **argv)
{
	int		i;
	char		*fullName=NULL;

	parseCommandLine(argc, argv);
	if (programMode == MODE_PRINT_SIZES)
	{
		std::cout <<"Sizes of the types:\n"
			"\tindex header "
			<<sizeof(struct zui::sHeader) <<".\n"
			"\tdevice header record "
			<<sizeof(struct zui::device::sHeader) <<".\n"
			"\tdriver header record "
			<<sizeof(struct zui::driver::sHeader) <<".\n"
			"\tregion record "
			<<sizeof(struct zui::driver::sRegion) <<".\n"
			"\tmessage record "
			<<sizeof(struct zui::driver::sMessage) <<".\n";

		exit(EXIT_SUCCESS);
	};

	// Check to see if the index directory exists.
	if (!folderExists(indexPath))
	{
		exit(printAndReturn(
				argv[0], "Index path invalid, or not a folder",
				EX_INVALID_INDEX_PATH));
	};

	if (programMode != MODE_ADD && programMode != MODE_CREATE)
	{
		exit(printAndReturn(
				argv[0], "Only ADD and CREATE modes are "
				"supported for now", EX_GENERAL));
	};

	// Create the new index files and exit.
	if (programMode == MODE_CREATE) {
		exit(createMode(argc, argv));
	};

	/* For any mode other than CREATE and PRINT_SIZES, there must be a
	 * valid index already in existence.
	 **/
	if (programMode == MODE_ADD || programMode == MODE_LIST
		|| programMode == MODE_REMOVE)
	{
		for (i=0; indexFileNames[i] != NULL; i++)
		{
			fullName = makeFullName(
				fullName, indexPath, indexFileNames[i]);

			if (fullName == NULL)
			{
				exit(printAndReturn(
					argv[0], "Out of memory", EX_GENERAL));
			};

			if (!fileExists(fullName))
			{
				exit(printAndReturn(
					argv[0], "No index found.\n"
					"Try options -c <endianness>",
					EX_NO_INDEX));
			};
		};
	};

	// Only check to see if the base path exists for ADD.
	if (programMode == MODE_ADD)
	{
		if (!ignoreInvalidBasePath && !folderExists(basePath))
		{
			std::cerr <<argv[0] <<"Warning: Base path \"" <<basePath <<"\" does "
				"not exist or is not a folder.\n";
		};

		exit(addMode(argc, argv));
	};

	exit(EX_UNKNOWN);
}

