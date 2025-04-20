#ifndef _DEBUG_PIPE_H
	#define _DEBUG_PIPE_H

	#include <stdarg.h>
	#include <arch/paddr_t.h>
	#include <arch/paging.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/debugBuffer.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>

/**	EXPLANATION:
 * The purpose of the debug pipe is to provide a uniform debug stream output
 * to the kernel to display boot information messages and errors.
 *
 * This interface directly connects to the firmwareTributary and prints
 * to the devices there, depending on what you connect it to. That is, the
 * kernel debug output is attached to one of four devices, all of which are
 * completely chipset dependent. A chipset may define one, all, or none, or
 * any other combination. Of course, not having an underlying device to print
 * to means obviously that you'll be sending all data to the debug buffer, and
 * not seeing it anywhere.
 *
 * At any time, the debug pipe can be 'untied' from any of these, including the
 * buffer. The buffer acts as a big sprintf() to store all messages. As soon as
 * possible, the kernel allocates the buffer, and begins printing to it.
 *
 * Subsequently, whenever the firmwareTributary is initialize()d, the kernel
 * can then tie the debug buffer to any number of devices of the four. This will
 * continue until the ekfs is loaded, at which point, the kernel unties from all
 * devices except for the buffer.
 *
 * The buffer is provided by the DebugPipe class itself, and maintained within
 * the class. It is resized in page-sized increments and it allocates these
 * from the kernel memory stream.
 *
 * It is intended that if the debug pipe runs out of memory (tries to allocate
 * and fails) it will just scroll the current buffer up and keep writing to old
 * memory. The class will eventually also have a "flush()" method to cause it
 * to relinquish all of its pages to the kernel.
 **/

#define DEBUGPIPE_DEVICE1			(1<<0)
#define DEBUGPIPE_DEVICE2			(1<<1)
#define DEBUGPIPE_DEVICE3			(1<<2)
#define DEBUGPIPE_DEVICE4			(1<<3)
#define DEBUGPIPE_DEVICE_BUFFER			(1<<4)

#define DEBUGPIPE_CONVERSION_BUFF_NBYTES	(4096)

#define NOTICE				CC"N: "
#define WARNING				CC"W: "
#define ERROR				CC"E: "
#define FATAL				CC"F: "

#define OPTS(__opts)			"%-" __opts " "
#define NOLOG				"n"

#define LOGONCE(__id)			NOLOG
#define LOGONCE_FINDTABLES(__id)

class DebugPipe
{
public:
	DebugPipe(void);
	error_t initialize(void);
	~DebugPipe(void);

public:
	// Zambesii only supports UTF-8 strings in the kernel.
	sarch_t printf(const utf8Char *str, va_list v);

	/**	NOTE:
	 * This version of printf is used for kernel debugging inside of
	 * spinlocks, and when debugging code whose cause of error may actually
	 * be the debugPipe itself.
	 *
	 * If the deadlock in the kernel seems to be on the debugPipe internal
	 * UTF8-expansion buffer lock, then this version can be used to pass
	 * a separate buffer for use by the debugPipe for the duration of that
	 * printf() call. It bypasses the internal lock and uses another one
	 * passed as an argument. This may cause a race condition on the kernel
	 * debug log, but seeing some output is better than seeing none.
	 **/
	sarch_t printf(SharedResourceGroup<WaitLock, utf8Char *> *buff,
		uarch_t buffSize, utf8Char *str, va_list v);

	/**	EXPLANATION:
	 * Can take more than one device per call (hence the bitfield form).
	 * Will return a bitfield containing the internal device tie state.
	 *
	 * So that means that if you ask it to tie to device 1, and it returns
	 * a bitmap with device1 unset, then obviously the device failed to
	 * initialize.
	 **/
	uarch_t tieTo(uarch_t device);
	uarch_t untieFrom(uarch_t device);

	// Refresh the buffer into all currently tied devices.
	void refresh(void);
	// Relinquish all pages in the buffer to the MM.
	void flush(void);

private:
	DebugBuffer		debugBuff;
	// 'convBuff' is used to expand the printf formatting.
	SharedResourceGroup<
		WaitLock,
		utf8Char[DEBUGPIPE_CONVERSION_BUFF_NBYTES]>	convBuff;

	SharedResourceGroup<WaitLock, uarch_t>	devices;
};

sarch_t printf(const utf8Char *str, ...);
sarch_t vprintf(const utf8Char *str, va_list args);

// The kernel deliberately does not provide sprintf.
extern "C" sarch_t snprintf(
	utf8Char *buff, uarch_t maxLength, utf8Char *format, ...);

// Used for debugging, see above.
sarch_t printf(
	SharedResourceGroup<WaitLock, utf8Char *> *buff,
	uarch_t buffSize, utf8Char *str, ...);

// Used for debugging with va_list
sarch_t vnprintf(
	SharedResourceGroup<WaitLock, utf8Char *> *buff,
	uarch_t buffSize, utf8Char *str, va_list args);

extern DebugPipe	__kdebug;

#endif

