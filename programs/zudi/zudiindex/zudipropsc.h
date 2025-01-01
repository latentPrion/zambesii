#ifndef _Z_UDIPROPS_COMPILER_H
	#define _Z_UDIPROPS_COMPILER_H

	#include <stdio.h>
	#include <stdlib.h>
	#include <zui.h>

enum parseModeE { PARSE_NONE, PARSE_TEXT, PARSE_BINARY };
enum programModeE {
	MODE_NONE, MODE_ADD, MODE_LIST, MODE_REMOVE, MODE_PRINT_SIZES,
	MODE_CREATE };

enum propsTypeE { DRIVER_PROPS, META_PROPS };

enum exitStatusE {
	EX_SUCCESS=EXIT_SUCCESS, EX_BAD_COMMAND_LINE, EX_INVALID_INDEX_PATH,
	EX_GENERAL, EX_UNKNOWN, EX_NO_REQUIRES_UDI, EX_INVALID_INPUT_FILE,
	EX_PARSE_ERROR, EX_NO_INDEX, EX_NOMEM, EX_FILE_OPEN, EX_FILE_IO };

extern enum parseModeE		parseMode;
extern enum programModeE	programMode;
extern enum propsTypeE		propsType;
extern int			hasRequiresUdi, hasRequiresUdiPhysio,
				verboseMode;
extern const char		*basePath, *indexPath;
extern char			verboseBuff[];

char *makeFullName(char *reallocMem, const char *path, const char *fileName);
inline static int printAndReturn(char *progname, const char *msg, int errcode)
{
	fprintf(stderr, "%s: %s.\n", progname, msg);
	return errcode;
}

enum parser_lineTypeE {
	LT_UNKNOWN=0, LT_INVALID, LT_OVERFLOW, LT_LIMIT_EXCEEDED, LT_MISC,
	LT_DRIVER, LT_MODULE, LT_REGION,
	LT_DEVICE, LT_MESSAGE, LT_DISASTER_MESSAGE, LT_MESSAGE_FILE,
	LT_CHILD_BOPS, LT_INTERNAL_BOPS, LT_PARENT_BOPS,
	LT_METALANGUAGE, LT_READABLE_FILE, LT_RANK, LT_PROVIDES };

int parser_initializeNewDriverState(uint16_t driverId);
struct zui::driver::sDriver *parser_getCurrentDriverState(void);
int parser_getNSupportedDevices(void);
int parser_getNSupportedMetas(void);
void parser_releaseState(void);

enum parser_lineTypeE parser_parseLine(const char *line, void **ret);

void index_initialize(void);
int index_insert(enum parser_lineTypeE lineType, void *obj);
int index_writeToDisk(void);
void index_free(void);

int incrementNRecords(void);

#endif

