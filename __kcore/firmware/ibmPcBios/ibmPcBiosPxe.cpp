#include <config.h>
#include <firmware/ibmPcBios/ibmPcBiosPxe.h>

#include <chipset/memoryAreas.h>
#include <__kclasses/debugPipe.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kclib/string8.h>
#if defined(CONFIG_FIRMWARE_IBM_PC_BIOS)
#include <firmware/ibmPcBios/ibmPcBios_coreFuncs.h>
#include <firmware/ibmPcBios/x86emu.h>
#endif

#define BIOSPXE		"ibmPcBiosPxe: "
#define PXE_STRUCT_MIN_LEN	0x2C
#define PXE_PM_ENTRY_OFF	0x10
#define LOWMEM_TX_HDR_OFF	0x88000
#define LOWMEM_TX_FRAME_OFF	0x89000
#define LOWMEM_MAX_TX_LEN	1400
#define ZBZ_FRAME_HDR_LEN	4
#define ZBZ_MAX_PAYLOAD_LEN	(LOWMEM_MAX_TX_LEN - sizeof(sEthHdr) - ZBZ_FRAME_HDR_LEN)
#define ZBZ_FRAME_MAGIC0	0x5A
#define ZBZ_FRAME_MAGIC1	0x62
#define ZBZ_FRAME_VERSION	0x01
#define ZBZ_MSG_SESSION_START	0x01
#define ZBZ_MSG_LOG		0x02
#define PXE_OPCODE_UNDI_TRANSMIT	0x0008
#define PXE_OPCODE_START_UNDI		0x0000
#define PXE_OPCODE_UNDI_INITIALIZE	0x0003
#define PXE_OPCODE_UNDI_OPEN		0x0006
#define PXE_STATUS_SUCCESS		0
#define ROM_OFF_UNDI_PTR		0x16
#define ROM_OFF_PCI_BUSDEVFN		0x375
#define ROM_OFF_BEV			0x386
#define UNDI_OFF_LOADER			0x0A
#define UNDI_OFF_CODE_SIZE		0x10
#define UNDI_OFF_DATA_SIZE		0x12
#define LOWMEM_UNDI_LOADER_OFF		0x8000
#define LOWMEM_PXE_API_OFF		0x8100
#define LOWMEM_UNDI_STUB_OFF		0x580
#define FLTR_DIRECTED			0x0001
#define FLTR_BRDCST			0x0002

namespace
{
struct sEthHdr
{
	ubit8	dst[6];
	ubit8	src[6];
	ubit16	ethertype;
} __attribute__((packed));

struct sPxeFar32
{
	ubit32	off;
	ubit16	sel;
} __attribute__((packed));

struct sPxeUndiTransmit
{
	ubit16	status;
	ubit16	protocol;
	ubit16	xmitFlag;
	ubit16	destSeg;
	ubit16	destOff;
	ubit16	tbdSeg;
	ubit16	tbdOff;
	ubit16	dataBlockCount;
	ubit16	dataBlockSeg;
	ubit16	dataBlockOff;
	ubit16	dataBlockLen;
} __attribute__((packed));

struct sUndiLoader
{
	ubit16	status;
	ubit16	ax;
	ubit16	bx;
	ubit16	dx;
	ubit16	di;
	ubit16	es;
	ubit16	undiDs;
	ubit16	undiCs;
	ubit16	pxeOff;
	ubit16	pxeSeg;
	ubit16	pxenvOff;
	ubit16	pxenvSeg;
} __attribute__((packed));

struct sPxeStartUndi
{
	ubit16	status;
	ubit16	ax;
	ubit16	bx;
	ubit16	dx;
	ubit16	di;
	ubit16	es;
} __attribute__((packed));

struct sPxeUndiInitialize
{
	ubit16	status;
	ubit32	protocolIni;
	ubit8	reserved[8];
} __attribute__((packed));

struct sPxeUndiOpen
{
	ubit16	status;
	ubit16	openFlag;
	ubit16	pktFilter;
} __attribute__((packed));
}

IbmPcBiosPxe		ibmPcBiosPxeDebug;

IbmPcBiosPxe::IbmPcBiosPxe(void)
:
ZkcmDebugDevice(ZkcmDebugDevice::NETWORK, &baseDeviceInfo),
s(CC"ibmPcBiosPxe state"),
baseDeviceInfo(
	0,
	CC"ibmpc-bios-pxe", CC"IBM PC BIOS PXE debug network sink",
	CC"Unknown vendor", CC"N/A")
{
	memset(&s.rsrc, 0, sizeof(s.rsrc));
}

error_t IbmPcBiosPxe::initialize(void)
{
#if !defined(CONFIG_ARCH_x86_32)
	return ERROR_UNSUPPORTED;
#else
	error_t ret;

	s.lock.acquire();
	if (s.rsrc.initialized)
	{
		s.lock.release();
		return ERROR_SUCCESS;
	}

	ret = configureTargets();
	if (ret != ERROR_SUCCESS)
	{
		s.lock.release();
		return ret;
	}

	ret = discoverPxe();
	if (ret == ERROR_SUCCESS)
	{
		s.rsrc.initialized = 1;
		printf(NOTICE BIOSPXE"Found !PXE with PM entry selector 0x%x "
			"offset 0x%x.\n",
			s.rsrc.pxeEntrySel, s.rsrc.pxeEntryOff);
	}

	s.lock.release();
	return ret;
#endif
}

error_t IbmPcBiosPxe::shutdown(void)
{
	s.lock.acquire();
	s.rsrc.initialized = 0;
	s.rsrc.lowmem = NULL;
	s.rsrc.pxeEntryOff = 0;
	s.rsrc.pxeEntrySel = 0;
	s.lock.release();
	return ERROR_SUCCESS;
}

error_t IbmPcBiosPxe::suspend(void) { return ERROR_SUCCESS; }
error_t IbmPcBiosPxe::restore(void) { return ERROR_SUCCESS; }
void IbmPcBiosPxe::clear(void) {}

sarch_t IbmPcBiosPxe::isInitialized(void)
{
	sarch_t ret;

	s.lock.acquire();
	ret = (s.rsrc.initialized != 0);
	s.lock.release();
	return ret;
}

void IbmPcBiosPxe::announceSessionStart(void)
{
	utf8Char metadata[64];

	snprintf(
		metadata, sizeof(metadata),
		CC"Zambesii %s", CC PACKAGE_VERSION);

	s.lock.acquire();
	if (s.rsrc.initialized)
	{
		sendZbzFrame(
			ZBZ_MSG_SESSION_START,
			reinterpret_cast<const ubit8 *>(metadata),
			strnlen8(metadata, sizeof(metadata) - 1));
	}
	s.lock.release();
}

void IbmPcBiosPxe::syphon(const utf8Char *str, uarch_t len)
{
	s.lock.acquire();
	if (!s.rsrc.initialized || str == NULL || len == 0)
	{
		s.lock.release();
		return;
	}

	while (len > 0)
	{
		uarch_t chunk = (len > ZBZ_MAX_PAYLOAD_LEN)
			? ZBZ_MAX_PAYLOAD_LEN : len;
		if (sendZbzFrame(
			ZBZ_MSG_LOG,
			reinterpret_cast<const ubit8 *>(str), chunk)
			!= ERROR_SUCCESS)
		{
			s.lock.release();
			return;
		}
		str += chunk;
		len -= chunk;
	}
	s.lock.release();
}

error_t IbmPcBiosPxe::discoverPxe(void)
{
	ubit8	*pxe;

	if (chipsetMemAreas::mapArea(CHIPSET_MEMAREA_LOWMEM) != ERROR_SUCCESS)
	{
		printf(WARNING BIOSPXE"Unable to map low memory area.\n");
		return ERROR_MEMORY_VIRTUAL_PAGEMAP;
	}

	s.rsrc.lowmem = reinterpret_cast<ubit8 *>(
		chipsetMemAreas::getArea(CHIPSET_MEMAREA_LOWMEM));
	if (s.rsrc.lowmem == NULL) { return ERROR_NOT_FOUND; }

	pxe = reinterpret_cast<ubit8 *>(findPxeSignature(s.rsrc.lowmem));
	if (pxe != NULL)
	{
		return validatePxeStruct(pxe);
	}

	printf(NOTICE BIOSPXE"No !PXE in RAM; trying UNDI loader path.\n");
	return loadViaUndiLoader();
}

error_t IbmPcBiosPxe::validatePxeStruct(ubit8 *pxe)
{
	ubit8	csum = 0;
	ubit8	len;

	len = pxe[4];
	if (len < PXE_STRUCT_MIN_LEN)
	{
		printf(WARNING BIOSPXE"!PXE struct length too small: %d.\n", len);
		return ERROR_NOT_FOUND;
	}

	for (ubit8 i = 0; i < len; i++) { csum += pxe[i]; }
	if (csum != 0)
	{
		printf(WARNING BIOSPXE"!PXE checksum failed (%d).\n", csum);
		return ERROR_NOT_FOUND;
	}

	memcpy(
		&s.rsrc.pxeEntryOff, &pxe[PXE_PM_ENTRY_OFF],
		sizeof(s.rsrc.pxeEntryOff));
	memcpy(
		&s.rsrc.pxeEntrySel, &pxe[PXE_PM_ENTRY_OFF + 4],
		sizeof(s.rsrc.pxeEntrySel));

	if (s.rsrc.pxeEntrySel == 0 || s.rsrc.pxeEntryOff == 0)
	{
		printf(WARNING BIOSPXE"!PXE PM entry was empty.\n");
		return ERROR_NOT_FOUND;
	}

	return ERROR_SUCCESS;
}

sbit8 IbmPcBiosPxe::findUndiRom(
	ubit8 *lowmem, uarch_t *romBase, uarch_t *loaderOff,
	ubit16 *codeSize, ubit16 *dataSize
	)
{
	for (uarch_t rom = 0xC0000; rom < 0xF0000; )
	{
		ubit8	*hdr = &lowmem[rom];
		ubit8	*undi;
		ubit16	undiRel;
		ubit16	romLen;

		if (hdr[0] != 0x55 || hdr[1] != 0xAA) {
			rom += 512;
			continue;
		}

		romLen = hdr[2];
		if (romLen == 0) {
			rom += 512;
			continue;
		}

		undiRel = static_cast<ubit16>(
			hdr[ROM_OFF_UNDI_PTR] | (hdr[ROM_OFF_UNDI_PTR + 1] << 8));
		if (undiRel == 0 || (rom + undiRel + 0x14) >= 0x100000) {
			rom += static_cast<uarch_t>(romLen) * 512;
			continue;
		}

		undi = &lowmem[rom + undiRel];
		if (strncmp8(CC(undi), CC"UNDI", 4) != 0) {
			rom += static_cast<uarch_t>(romLen) * 512;
			continue;
		}

		*romBase = rom;
		*loaderOff = static_cast<uarch_t>(
			undi[UNDI_OFF_LOADER] | (undi[UNDI_OFF_LOADER + 1] << 8));
		*codeSize = static_cast<ubit16>(
			undi[UNDI_OFF_CODE_SIZE] | (undi[UNDI_OFF_CODE_SIZE + 1] << 8));
		*dataSize = static_cast<ubit16>(
			undi[UNDI_OFF_DATA_SIZE] | (undi[UNDI_OFF_DATA_SIZE + 1] << 8));

		return 1;
	}

	return 0;
}

ubit16 IbmPcBiosPxe::readRomPciBdf(ubit8 *lowmem, uarch_t romBase)
{
	ubit16	bdf;

	bdf = static_cast<ubit16>(
		lowmem[romBase + ROM_OFF_PCI_BUSDEVFN]
		| (lowmem[romBase + ROM_OFF_PCI_BUSDEVFN + 1] << 8));
	if (bdf != 0) { return bdf; }

	/* QEMU e1000 on bus 0 dev 3 fn 0 when no BDF was stored in ROM. */
	return 0x0018;
}

ubit16 IbmPcBiosPxe::findPnpCheckOffset(ubit8 *lowmem)
{
	for (uarch_t off = 0xF0000; off < 0x100000; off += 16)
	{
		if (strncmp8(CC(&lowmem[off]), CC"$PnP", 4) == 0) {
			return static_cast<ubit16>(off & 0xFFFF);
		}
	}

	return 0;
}

#if defined(CONFIG_FIRMWARE_IBM_PC_BIOS)
static void ibmPcBiosPxe_writeEmuStub(
	ubit8 *emuMem, uarch_t stubOff, ubit16 callOff, ubit16 callSeg,
	sbit8 pushParams
	)
{
	ubit8	*stub = &emuMem[stubOff];
	ubit8	i = 0;

	if (pushParams)
	{
		stub[i++] = 0x66; /* pushl %ebp (iPXE stack layout) */
		stub[i++] = 0x55;
		stub[i++] = 0x1E; /* push %ds */
		stub[i++] = 0x50; /* push %ax */
	}

	stub[i++] = 0x9A; /* lcall */
	stub[i++] = static_cast<ubit8>(callOff);
	stub[i++] = static_cast<ubit8>(callOff >> 8);
	stub[i++] = static_cast<ubit8>(callSeg);
	stub[i++] = static_cast<ubit8>(callSeg >> 8);
	stub[i++] = 0xF4; /* hlt */
}

static void ibmPcBiosPxe_runEmuAt(uarch_t entryOff, ubit16 ds, ubit16 ax)
{
	*(ubit32 *)(reinterpret_cast<ubit8 *>(M.mem_base) + entryOff + 16)
		= 0xF4F4F4F4;

	M.x86.R_CS = 0;
	M.x86.R_EIP = entryOff;
	M.x86.R_SS = 0;
	M.x86.R_ESP = 0x7E00;
	M.x86.R_DS = ds;
	M.x86.R_ES = ds;
	M.x86.R_AX = ax;
	M.x86.R_EFLG = 0x202;
	M.x86.intr = 0;

	X86EMU_exec();
}
#endif

error_t IbmPcBiosPxe::loadViaUndiLoader(void)
{
#if !defined(CONFIG_FIRMWARE_IBM_PC_BIOS)
	return ERROR_UNSUPPORTED;
#else
	uarch_t		romBase, loaderOff;
	ubit16		codeSize, dataSize;
	ubit16		romSeg, loaderSeg;
	ubit16		fbmsKb, fbmsSeg;
	ubit16		pciBdf;
	sUndiLoader	*loader;
	ubit8		*pxe;
	error_t		ret;
	ubit8		*emuMem;

	if (!findUndiRom(
		s.rsrc.lowmem, &romBase, &loaderOff, &codeSize, &dataSize))
	{
		printf(WARNING BIOSPXE"No UNDI expansion ROM found.\n");
		return ERROR_NOT_FOUND;
	}

	if (loaderOff == 0 || codeSize == 0) {
		printf(WARNING BIOSPXE"UNDI ROM header was incomplete.\n");
		return ERROR_NOT_FOUND;
	}

	pciBdf = readRomPciBdf(s.rsrc.lowmem, romBase);

	fbmsKb = static_cast<ubit16>(
		s.rsrc.lowmem[0x413] | (s.rsrc.lowmem[0x414] << 8));
	fbmsSeg = static_cast<ubit16>(fbmsKb << 6);
	fbmsSeg -= static_cast<ubit16>((codeSize + 0x0F) >> 4);

	loader = reinterpret_cast<sUndiLoader *>(
		&s.rsrc.lowmem[LOWMEM_UNDI_LOADER_OFF]);
	memset(loader, 0, sizeof(*loader));
	loader->ax = pciBdf;
	loader->bx = 0xFFFF;
	loader->dx = 0xFFFF;
	loader->es = 0xF000;
	loader->di = findPnpCheckOffset(s.rsrc.lowmem);
	loader->undiCs = fbmsSeg;
	loader->undiDs = static_cast<ubit16>(
		fbmsSeg - static_cast<ubit16>((dataSize + 0x0F) >> 4));

	ret = ibmPcBios::initialize();
	if (ret != ERROR_SUCCESS) {
		printf(WARNING BIOSPXE"BIOS emulator init failed.\n");
		return ret;
	}

	ibmPcBios::acquireLock();

	romSeg = static_cast<ubit16>(romBase >> 4);
	loaderSeg = romSeg;
	emuMem = reinterpret_cast<ubit8 *>(M.mem_base);
	memcpy(&emuMem[LOWMEM_UNDI_LOADER_OFF], loader, sizeof(*loader));

	ibmPcBiosPxe_writeEmuStub(
		emuMem, LOWMEM_UNDI_STUB_OFF,
		static_cast<ubit16>(loaderOff), loaderSeg, 1);
	ibmPcBiosPxe_runEmuAt(
		LOWMEM_UNDI_STUB_OFF,
		static_cast<ubit16>(LOWMEM_UNDI_LOADER_OFF >> 4), 0);

	memcpy(loader, &emuMem[LOWMEM_UNDI_LOADER_OFF], sizeof(*loader));

	if (loader->pxeSeg == 0 || loader->pxeOff == 0)
	{
		printf(NOTICE BIOSPXE"UNDI loader returned no PXEptr; trying BEV/exec.\n");

		ibmPcBiosPxe_writeEmuStub(
			emuMem, LOWMEM_UNDI_STUB_OFF,
			ROM_OFF_BEV, romSeg, 0);
		ibmPcBiosPxe_runEmuAt(LOWMEM_UNDI_STUB_OFF, 0, 0);

		pxe = reinterpret_cast<ubit8 *>(findPxeSignature(s.rsrc.lowmem));
		ibmPcBios::releaseLock();
		if (pxe == NULL) {
			printf(WARNING BIOSPXE"BEV/exec did not publish !PXE.\n");
			return ERROR_NOT_FOUND;
		}

		ret = validatePxeStruct(pxe);
		if (ret != ERROR_SUCCESS) { return ret; }
		return startUndiStack(romBase, pciBdf);
	}

	ibmPcBios::releaseLock();
	pxe = &s.rsrc.lowmem[
		(static_cast<uarch_t>(loader->pxeSeg) << 4) + loader->pxeOff];

	if (strncmp8(CC(pxe), CC"!PXE", 4) != 0) {
		printf(WARNING BIOSPXE"UNDI loader did not publish !PXE.\n");
		return ERROR_NOT_FOUND;
	}

	ret = validatePxeStruct(pxe);
	if (ret != ERROR_SUCCESS) { return ret; }

	return startUndiStack(romBase, pciBdf);
#endif
}

error_t IbmPcBiosPxe::startUndiStack(uarch_t romBase, ubit16 pciBdf)
{
	sPxeStartUndi		*start;
	sPxeUndiInitialize	*init;
	sPxeUndiOpen		*open;
	error_t			ret;

	(void)romBase;

	start = reinterpret_cast<sPxeStartUndi *>(
		&s.rsrc.lowmem[LOWMEM_PXE_API_OFF]);
	memset(start, 0, sizeof(*start));
	start->ax = pciBdf;
	start->bx = 0xFFFF;
	start->dx = 0xFFFF;
	start->es = 0xF000;
	start->di = findPnpCheckOffset(s.rsrc.lowmem);

	ret = callPxeApi(PXE_OPCODE_START_UNDI, LOWMEM_PXE_API_OFF);
	if (ret != ERROR_SUCCESS || start->status != PXE_STATUS_SUCCESS) {
		printf(WARNING BIOSPXE"PXENV_START_UNDI failed (0x%x).\n",
			start->status);
		return ERROR_NOT_FOUND;
	}

	init = reinterpret_cast<sPxeUndiInitialize *>(
		&s.rsrc.lowmem[LOWMEM_PXE_API_OFF + 0x40]);
	memset(init, 0, sizeof(*init));

	ret = callPxeApi(PXE_OPCODE_UNDI_INITIALIZE, LOWMEM_PXE_API_OFF + 0x40);
	if (ret != ERROR_SUCCESS || init->status != PXE_STATUS_SUCCESS) {
		printf(WARNING BIOSPXE"PXENV_UNDI_INITIALIZE failed (0x%x).\n",
			init->status);
		return ERROR_NOT_FOUND;
	}

	open = reinterpret_cast<sPxeUndiOpen *>(
		&s.rsrc.lowmem[LOWMEM_PXE_API_OFF + 0x80]);
	memset(open, 0, sizeof(*open));
	open->openFlag = 0;
	open->pktFilter = FLTR_DIRECTED | FLTR_BRDCST;

	ret = callPxeApi(PXE_OPCODE_UNDI_OPEN, LOWMEM_PXE_API_OFF + 0x80);
	if (ret != ERROR_SUCCESS || open->status != PXE_STATUS_SUCCESS) {
		printf(WARNING BIOSPXE"PXENV_UNDI_OPEN failed (0x%x).\n",
			open->status);
		return ERROR_NOT_FOUND;
	}

	return ERROR_SUCCESS;
}

error_t IbmPcBiosPxe::callPxeApi(ubit16 opcode, uarch_t paramPhys)
{
#if !defined(CONFIG_ARCH_x86_32)
	(void)opcode;
	(void)paramPhys;
	return ERROR_UNSUPPORTED;
#else
	sPxeFar32	entry;
	ubit16		seg, off;
	ubit16		bx = opcode;

	if (s.rsrc.pxeEntrySel == 0 || s.rsrc.pxeEntryOff == 0) {
		return ERROR_NOT_FOUND;
	}

	entry.off = s.rsrc.pxeEntryOff;
	entry.sel = s.rsrc.pxeEntrySel;

	off = static_cast<ubit16>(paramPhys & 0xF);
	seg = static_cast<ubit16>(paramPhys >> 4);

	asm volatile(
		"push %%es\n\t"
		"mov %2, %%es\n\t"
		"lcall *%3\n\t"
		"pop %%es\n\t"
		: "+b"(bx)
		: "D"(off), "rm"(seg), "m"(entry)
		: "cc", "memory");

	(void)bx;
	return ERROR_SUCCESS;
#endif
}

error_t IbmPcBiosPxe::configureTargets(void)
{
	if (parseMac(CC CONFIG_DEBUGPIPE_BIOS_PXE_TARGET_MAC, s.rsrc.dstMac)
		!= ERROR_SUCCESS)
	{
		printf(WARNING BIOSPXE"Invalid target MAC '%s'.\n",
			CC CONFIG_DEBUGPIPE_BIOS_PXE_TARGET_MAC);
		return ERROR_INVALID_ARG_VAL;
	}

	// Source MAC is locally administered and static for now.
	s.rsrc.srcMac[0] = 0x02;
	s.rsrc.srcMac[1] = 0x5a;
	s.rsrc.srcMac[2] = 0x6d;
	s.rsrc.srcMac[3] = 0x62;
	s.rsrc.srcMac[4] = 0x73;
	s.rsrc.srcMac[5] = 0x69;
	return ERROR_SUCCESS;
}

error_t IbmPcBiosPxe::sendZbzFrame(
	ubit8 type, const ubit8 *payload, uarch_t payloadLen
	)
{
	uarch_t	frameLen;
	ubit8	*frame;
	sEthHdr	*eth;
	ubit8	*zbz;

	if (s.rsrc.lowmem == NULL) { return ERROR_INVALID_ARG_VAL; }
	if (payload == NULL && payloadLen != 0) { return ERROR_INVALID_ARG_VAL; }
	if (payloadLen > ZBZ_MAX_PAYLOAD_LEN) { return ERROR_INVALID_ARG_VAL; }

	frameLen = sizeof(sEthHdr) + ZBZ_FRAME_HDR_LEN + payloadLen;

	frame = &s.rsrc.lowmem[LOWMEM_TX_FRAME_OFF];
	memset(frame, 0, frameLen);

	eth = reinterpret_cast<sEthHdr *>(frame);
	memcpy(eth->dst, s.rsrc.dstMac, sizeof(eth->dst));
	memcpy(eth->src, s.rsrc.srcMac, sizeof(eth->src));
	eth->ethertype = toBe16(CONFIG_DEBUGPIPE_BIOS_PXE_ETHERTYPE);

	zbz = frame + sizeof(*eth);
	zbz[0] = ZBZ_FRAME_MAGIC0;
	zbz[1] = ZBZ_FRAME_MAGIC1;
	zbz[2] = ZBZ_FRAME_VERSION;
	zbz[3] = type;

	if (payloadLen > 0)
	{
		memcpy(zbz + ZBZ_FRAME_HDR_LEN, payload, payloadLen);
	}

	return undiTransmit(frame, frameLen);
}

error_t IbmPcBiosPxe::undiTransmit(const ubit8 *frame, uarch_t frameLen)
{
#if !defined(CONFIG_ARCH_x86_32)
	(void)frame;
	(void)frameLen;
	return ERROR_UNSUPPORTED;
#else
	sPxeUndiTransmit	*tx;
	sPxeFar32		entry;
	ubit16			seg;
	ubit16			off;
	ubit16			bx = PXE_OPCODE_UNDI_TRANSMIT;

	if (s.rsrc.lowmem == NULL || frame == NULL || frameLen == 0) {
		return ERROR_INVALID_ARG_VAL;
	}

	memcpy(&s.rsrc.lowmem[LOWMEM_TX_FRAME_OFF], frame, frameLen);
	tx = reinterpret_cast<sPxeUndiTransmit *>(
		&s.rsrc.lowmem[LOWMEM_TX_HDR_OFF]);
	memset(tx, 0, sizeof(*tx));

	off = LOWMEM_TX_FRAME_OFF & 0xF;
	seg = LOWMEM_TX_FRAME_OFF >> 4;

	tx->protocol = 0; /* P_UNKNOWN: Ethernet header already in frame buffer */
	tx->xmitFlag = 0; /* XMT_DESTADDR: destination MAC is in frame */
	tx->destSeg = 0;
	tx->destOff = 0;
	tx->tbdSeg = 0;
	tx->tbdOff = 0;
	tx->dataBlockCount = 1;
	tx->dataBlockSeg = seg;
	tx->dataBlockOff = off;
	tx->dataBlockLen = frameLen;

	entry.off = s.rsrc.pxeEntryOff;
	entry.sel = s.rsrc.pxeEntrySel;

	off = LOWMEM_TX_HDR_OFF & 0xF;
	seg = LOWMEM_TX_HDR_OFF >> 4;

	asm volatile(
		"push %%es\n\t"
		"mov %2, %%es\n\t"
		"lcall *%3\n\t"
		"pop %%es\n\t"
		: "+b"(bx)
		: "D"(off), "rm"(seg), "m"(entry)
		: "cc", "memory");

	(void)bx;
	return ERROR_SUCCESS;
#endif
}

ubit16 IbmPcBiosPxe::toBe16(ubit16 val)
{
	return static_cast<ubit16>((val << 8) | (val >> 8));
}

static inline ubit8 ibmPcBiosPxe_hexVal(utf8Char c)
{
	if (c >= '0' && c <= '9') { return static_cast<ubit8>(c - '0'); }
	if (c >= 'a' && c <= 'f') { return static_cast<ubit8>(10 + c - 'a'); }
	if (c >= 'A' && c <= 'F') { return static_cast<ubit8>(10 + c - 'A'); }
	return 0xFF;
}

error_t IbmPcBiosPxe::parseMac(const utf8Char *in, ubit8 out[6])
{
	for (ubit8 i = 0; i < 6; i++)
	{
		ubit8 hi, lo;
		if (in[0] == '\0' || in[1] == '\0') {
			return ERROR_INVALID_ARG_VAL;
		}
		hi = ibmPcBiosPxe_hexVal(in[0]);
		lo = ibmPcBiosPxe_hexVal(in[1]);
		if (hi == 0xFF || lo == 0xFF) {
			return ERROR_INVALID_ARG_VAL;
		}
		out[i] = (hi << 4) | lo;
		in += 2;
		if (i == 5) { break; }
		if (*in != ':') { return ERROR_INVALID_ARG_VAL; }
		in++;
	}
	return (*in == '\0') ? ERROR_SUCCESS : ERROR_INVALID_ARG_VAL;
}

void *IbmPcBiosPxe::findPxeSignature(ubit8 *lowmem)
{
	for (uarch_t off = 0; off < 0x100000; off += 16)
	{
		ubit8 *p = &lowmem[off];
		if (strncmp8(CC(p), CC"!PXE", 4) == 0) {
			return p;
		}
	}

	return NULL;
}
