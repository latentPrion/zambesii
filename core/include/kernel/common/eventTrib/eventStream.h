#ifndef _EVENT_STREAM_H
	#define _EVENT_STREAM_H

	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/bitmap.h>
	#include <kernel/common/stream.h>

class eventStreamC
:
public streamC
{
public:
	eventStreamC(void);
	error_t initialize(void);

private:
};

#endif

