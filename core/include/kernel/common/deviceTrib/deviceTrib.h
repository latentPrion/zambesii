#ifndef _DEVICE_TRIBUTARY_H
	#define _DEVICE_TRIBUTARY_H

	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/ptrList.h>
	#include <kernel/common/tributary.h>

#define DEVICETRIB_TYPE_BUS		0x1
#define DEVICETRIB_TYPE_VIDEO		0x2
#define DEVICETRIB_TYPE_AUDIO		0x3
#define DEVICETRIB_TYPE_STORAGE		0x4
#define DEVICETRIB_TYPE_CHARINPUT	0x5
#define DEVICETRIB_TYPE_COORDINPUT	0x6
#define DEVICETRIB_TYPE_NETWORK		0x7
#define DEVICETRIB_TYPE_FILESYSTEM	0x8

class deviceTribC
:
public tributaryC
{
public:

private:
	ptrListC<busDevC>		bus;
	ptrListC<videoDevC>		video;
	ptrListC<audioDevC>		audio;
	ptrListC<storageDevC>		storage;
	ptrListC<networkDevC>		network;
	ptrListC<charInputDevC>		charInput;
	ptrListC<coordInputDevC>	coordInput;
};

#endif

