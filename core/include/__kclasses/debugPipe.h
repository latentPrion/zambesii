#ifndef _DEBUG_PIPE_H
	#define _DEBUG_PIPE_H

	#include <__kstdlib/__ktypes.h>

/**	EXPLANATION:
 * The purpose of the debug pipe is to provide a uniform debug stream output
 * to the kernel to display boot information messages and errors.
 *
 * This interface directly connects to the firmwareTributary and prints
 * to the devices there, depending on what you connect it to. That is, the
 * kernel debug output is attached to one or more devices, which include:
 *	1. A buffer,
 *	2. A screen.
 *	3. A serial interface.
 *	4. A parallel interface.
 *	5. An NIC.
 *
 * At any time, the debug pipe can be 'untied' from any of these, including the
 * buffer. The buffer acts as a big sprintf() to store all messages. As soon as
 * possible, the kernel allocates the buffer, and begins printing to it.
 *
 * Subsequently, whenever the firmwareTributary is initialize()d, the kernel
 * will then tie the debug buffer to the serial device and screen. This will
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

#define DEBUGPIPE_DEVICE_BUFFER		(1<<0)
#define DEBUGPIPE_DEVICE_SCREEN		(1<<1)
#define DEBUGPIPE_DEVICE_SERIAL		(1<<2)
#define DEBUGPIPE_DEVICE_PARALLEL	(1<<3)
#define DEBUGPIPE_DEVICE_NIC		(1<<4)

#define BUFFPAGE_MAX_NCHARS		\
	((PAGING_BASE_SIZE - sizeof(void *)) / sizeof(utf16Char))

class debugPipeC
{
public:
	debugPipeC(void);
	error_t initialize(void);
	~debugPipeC(void);

public:
	// Use L"string". Zambezii only supports UTF-16 strings.
	void printf(utf16Char *str, ...);

	// Can take more than one device per call (hence the bitfield form).
	error_t tieTo(uarch_t device);
	error_t untieFrom(uarch_t device);

	// Refresh the buffer into all currently tied devices.
	void refresh(void);
	// Relinquish all pages in the buffer to the MM.
	void flush(void);

private:
	// sizeof(buffPageS) should be exactly PAGING_BASE_SIZE or just below.
	struct buffPageS
	{
		buffPageS	*next;
		utf16Char	data[BUFFPAGE_MAX_NCHARS];
	} *buff;
	uarch_t devices;
}

extern debugPipeC	__kdebug;

#endif
