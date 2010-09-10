#ifndef _DEBUG_PIPE_H
	#define _DEBUG_PIPE_H

	#include <arch/paging.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kclib/stdarg.h>
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
 * The buffer is provided by the debugPipeC class itself, and maintained within
 * the class. It is resized in page-sized increments and it allocates these
 * from the kernel memory stream.
 *
 * It is intended that if the debug pipe runs out of memory (tries to allocate
 * and fails) it will just scroll the current buffer up and keep writing to old
 * memory. The class will eventually also have a "flush()" method to cause it
 * to relinquish all of its pages to the kernel.
 **/

#define DEBUGPIPE_DEVICE1		(1<<0)
#define DEBUGPIPE_DEVICE2		(1<<1)
#define DEBUGPIPE_DEVICE3		(1<<2)
#define DEBUGPIPE_DEVICE4		(1<<3)
#define DEBUGPIPE_DEVICE_BUFFER		(1<<4)

#define DEBUGPIPE_CONVERSION_BUFF_NPAGES	4

#define NOTICE				(utf8Char *)"N: "
#define WARNING				(utf8Char *)"W: "
#define ERROR				(utf8Char *)"E: "
#define FATAL				(utf8Char *)"F: "

class debugPipeC
{
public:
	debugPipeC(void);
	error_t initialize(void);
	~debugPipeC(void);

public:
	// Zambezii only supports UTF-8 strings in the kernel.
	void printf(const utf8Char *str, va_list v);

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
	void unsignedToStr(uarch_t num, uarch_t *len);
	void signedToStr(sarch_t num, uarch_t *len);
	void numToStrHex(uarch_t num, uarch_t *len);

	debugBufferC		debugBuff;
	// 'convBuff' is used to expand the passed UTF-8 string into codepoints.
	sharedResourceGroupC<waitLockC, unicodePoint *>	convBuff;
	sharedResourceGroupC<waitLockC, uarch_t>	devices;
};

void __kprintf(const utf8Char *str, ...);
extern debugPipeC	__kdebug;

#endif

