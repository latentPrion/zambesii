#ifndef _DEVICE_TRIBUTARY_H
	#define _DEVICE_TRIBUTARY_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/tributary.h>

class deviceTribC
:
public tributaryC
{
public:

private:
	busDevC		*bus,
	videoDevC	*video,
	audioDevC	*audio,
	storageDevC	*storage,
	networkDevC	*network,
	charInputDevC	*charInput,
	coordInputDevC	*coordInput,
};

#endif

