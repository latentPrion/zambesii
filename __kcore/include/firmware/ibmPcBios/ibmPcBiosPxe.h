#ifndef _IBM_PC_BIOS_PXE_DEBUG_DEVICE_H
	#define _IBM_PC_BIOS_PXE_DEBUG_DEVICE_H

	#include <chipset/zkcm/debugDevice.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>

class IbmPcBiosPxe
:
public ZkcmDebugDevice
{
public:
	IbmPcBiosPxe(void);

public:
	virtual error_t	initialize(void);
	virtual error_t	shutdown(void);
	virtual error_t	suspend(void);
	virtual error_t	restore(void);
	virtual sarch_t	isInitialized(void);
	virtual void	syphon(const utf8Char *str, uarch_t len);
	virtual void	clear(void);

	void		announceSessionStart(void);

private:
	error_t discoverPxe(void);
	error_t configureTargets(void);
	error_t sendZbzFrame(ubit8 type, const ubit8 *payload, uarch_t payloadLen);
	error_t undiTransmit(const ubit8 *frame, uarch_t frameLen);
	error_t validatePxeStruct(ubit8 *pxe);
	error_t loadViaUndiLoader(void);
	error_t startUndiStack(uarch_t romBase, ubit16 pciBdf);
	error_t callPxeApi(ubit16 opcode, uarch_t paramPhys);

private:
	static ubit16 toBe16(ubit16 val);
	static error_t parseMac(const utf8Char *in, ubit8 out[6]);
	static void *findPxeSignature(ubit8 *lowmem);
	static sbit8 findUndiRom(ubit8 *lowmem, uarch_t *romBase, uarch_t *loaderOff,
		ubit16 *codeSize, ubit16 *dataSize);
	static ubit16 readRomPciBdf(ubit8 *lowmem, uarch_t romBase);
	static ubit16 findPnpCheckOffset(ubit8 *lowmem);

private:
	struct sState
	{
		ubit8			initialized;
		ubit8			*lowmem;
		ubit32			pxeEntryOff;
		ubit16			pxeEntrySel;
		ubit8			dstMac[6];
		ubit8			srcMac[6];
	};
	SharedResourceGroup<WaitLock, sState>	s;

	ZkcmDevice			baseDeviceInfo;
};

extern IbmPcBiosPxe		ibmPcBiosPxeDebug;

#endif
