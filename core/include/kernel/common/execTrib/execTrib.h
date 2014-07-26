#ifndef _EXECUTABLE_FORMAT_TRIBUTARY_H
	#define _EXECUTABLE_FORMAT_TRIBUTARY_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/tributary.h>
	#include <kernel/common/execTrib/executableFormat.h>

/**	EXPLANATION:
 * The Executable Tributary is an abstraction layer that provides a uniform
 * method for parsing multiple executable formats.
 *
 * The tributary can load new executable format parsing libraries at runtime,
 * in the form of relocatable ELF modules. ELF is the kernel's default
 * executable format. The ELF parser lib is statically compiled into the kernel
 * image, and is a hardcoded index in this tributary's array of loaded format
 * parsers.
 *
 * The lib works by taking a buffer (which is expected to be read from some
 * file, or wherever), and testing that buffer to see what kind of executable
 * it is. The kernel doesn't however, care about the specifics of executable
 * formats: if it identifies the data passed to it as being conformant to some
 * executable format, it will simply return a handle to the loaded parser lib
 * that claimed to be able to parse it.
 *
 * From there, the caller takes that handle, and for all subsequent calls to
 * parse that library, it will call into that handle's entry points and expect
 * that that handle is a set of function pointers whose functions will
 * successfully parse the buffer it is passed.
 *
 * This buffer data passed to the parser library may be read from disk, or from
 * a network pipe, or may be resident in memory: the library does not care.
 **/

#define EXECTRIB		"Exec Trib: "

#define EXECTRIB_MAX_NPARSERS	16

#define EXECTRIB_PARSER_FLAGS_STATIC	(1<<0)

class ExecTrib
:
public Tributary
{
public:
	ExecTrib(void);
	error_t initialize(void);

public:
	executableParserS *identify(void *buff);

private:
	struct executableFormatS
	{
		executableParserS	*desc;
		uarch_t			flags;
	};
	executableFormatS	parsers[EXECTRIB_MAX_NPARSERS];
};

extern ExecTrib		execTrib;

#endif

