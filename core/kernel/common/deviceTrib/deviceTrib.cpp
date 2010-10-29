
#include <kernel/common/deviceTrib/deviceTrib.h>


error_t deviceTribC::addBus(busDevC *device)
{
	return bus.insert(device);
}

error_t deviceTribC::addVideo(videoDevC *dev)
{
	return video.insert(dev);
}

error_t deviceTribC::addAudio(audioDevC *dev)
{
	return audio.insert(dev);
}

error_t deviceTribC::addStorage(storageDevC *dev)
{
	return storage.insert(dev);
}

error_t deviceTribC::addNetwork(networkDevC *dev)
{
	return network.insert(dev);
}

error_t deviceTribC::addCharInput(charInputDevC *dev)
{
	return network.insert(dev);
}

error_t deviceTribC::addCoordInput(coordInputDevC *dev)
{
	return coordInput.insert(dev);
}

