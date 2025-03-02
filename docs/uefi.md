# Notes:

## Booting from storage devices:

UEFI doesn't require any modifications to the boot sector of a storage device. From Ch2 sec0:
> UEFI supports booting from media that contain an UEFI OS Loader or an UEFI-defined System Partition. An UEFI-defined System Partition is required by UEFI to boot from a block device.

## Bootloader load attempts:

UEFI will loop through all known OS loaders. For each loader:
* If loader calls Exit(), then UEFI moves on to next loader.
* If a BOOT#### option returns with EFI_SUCCESS, then "platform firmware supports boot manager menu, and if firmware is configured to boot in an interactive mode, the boot manager will stop processing the BootOrder variable and present a boot manager menu to the user." (sec 3.1.1).
* If neither of the above, then UEFI will loop through the next loader.
* If all loaders call Exit(), I suppose that means the system has failed to boot.
* If all possibilities are exhausted, then "boot option recovery must be performed" (sec 3.1.1).

The __OS itself__ is expected to call `ExitBootServices()` when it's ready to take over control.

More from sec 4.1.1:
> If the UEFI image is a UEFI OS Loader, then the UEFI OS Loader executes and either returns, calls the EFI Boot Service Exit() , or calls the EFI Boot Service EFI_BOOT_SERVICES.ExitBootServices() . If the EFI OS Loader returns or calls Exit() , then the load of the OS has failed, and the EFI OS Loader is unloaded from memory and control is returned to the component that attempted to boot the UEFI OS Loader

## NVRAM and global boot menu:

UEFI specifies a Boot Manager component that allows loading of any UEFI application, including OS Loaders; or drivers. UEFI defines NVRAM variables that point to the file to be loaded.

## Address space considerations:

* UEFI runtime services require that the OS "switch into a flat physical addressing mode to make the runtime call" (sec2.2.2). Would identity mapping be sufficient?
  * But, if you call SetVirtualAddressMap(), then "the OS will only be able to call runtime services in virtual addressing mode" (sec2.2.2).
* ConvertPointer(), Table2.2: Runtime service; may be relevant.
> Used to convert a pointer from physical to virtual addressing.
* UpdateCapsule(), Table2.2: Runtime service; may be relevant:
> Passes capsules to the firmware with both virtual and physical mapping.

Firmware may "request" that a memory region be given a virtual mapping via the EFI_MEMORY_DESCRIPTOR
having the EFI_MEMORY_RUNTIME bit set.
  * Any such mem region must be aligned 4KiB and be a multiple of 4KiB.


## Calling conventions:

### EFI service/protocol interface function pointers:

* Sec 2.3.2: EAX, ECX and EDX are volatile (callee-mutable) and caller-saved.

Sec 2.3: Apparently pointers passed as args must be physical addresses:
> It is the caller’s responsibility to pass pointer parameters that reference physical memory locations. If a pointer is passed that does not point to a physical memory location (i.e., a memory mapped I/O region), the results are unpredictable and the system may halt.

Passing pointer args to EFI functions:
> Unless otherwise stated, all functions defined in the UEFI specification are called through pointers in common, archi-tecturally defined, calling conventions found in C compilers.
  * NB: Linux uses __attribute((regparm(0))) to call EFI functions. Might be important.

### IA-32 calling convention:

Sec 2.3.2.2:
> All functions are called with the C language calling convention. The general-purpose registers that are volatile across function calls are eax , ecx , and edx . All other general-purpose registers are nonvolatile and are preserved by the target function.
> In addition, unless otherwise specified by the function definition, all other CPU registers (including MMX and XMM)are preserved.
> The floating point status register is not preserved by the target function. The floating point control register and MMX control register are saved by the target function.
> If the return value is a float or a double, the value is returned in ST(0).

## IA-32 Machine state prior to ExitBootServices():

* Uni-processor in Protected mode.
* Paging may or may not be enabled.
  * If enabled, then (sec 2.3.2):
> any memory space defined by the UEFI memory map is identity mapped (virtual address equals physical address). The mappings to other regions are undefined and may vary from implementation to implementation.
* GDT segments are flat and "are otherwise not used" (sec2.3.2).
* CPU Interrupts are enabled, though no ISRs are "supported" other than the Timer Boot Services functions.
* EFLAGS.DF is clear. Other GP regs are undefined.
* 128KiB stack "or more"; must be 16B aligned; may be marked NX.
* CR0.{TS, EM} are zero.

This state must be restored by the OS prior to calling boot and runtime services (sec 2.3.2):
> An application written to this specification may alter the processor execution mode, but the UEFI image must ensure firmware boot services and runtime services are executed with the prescribed execution environment.

## OS Loader handoff:

### IA-32:
UEFI invokes an OS Loader with the stack set up as follows (sec 2.3.2.1):
> ESP+0 = RETURN ADDRESS.\
> ESP+4 = IMAGE_HANDLE (used to get NVRAM var data)\
> ESP+8 = EFI_SYSTEM_TABLE pointer

### x86_64 (sec 2.3.5.1):
> R0 = EFI_HANDLE (image handle, most likely)\
> R1 = EFI_SYSTEM_TABLE pointer\
> R14 = Return address

## Runtime services:

* "All runtime interfaces are non-blocking interfaces and can be called with interrupts disabled" (sec2.2.2).

### Invoking runtime services:

Environment must be in following state when calling runtime services:
* CPU in protected mode (BSP, or can we call on APs?).
* Interrupts enabled or disabled.
* Paging may or may not be enabled, but if enabled then SetVirtualAddressMap() must have been called.
  * If SetVirtualAddressMap() wasn't called, then:
    * Page tables must be identity mapped.
    * "any memory space defined by the UEFI memory map is identity mapped (virtual address equals physical address)" (sec2.3.2).
* EFLAGS.DF is clear.
* 4KiB stack "or more"; must be 16B aligned; may be marked NX.
* FP control word = 027Fh; MMX control word = 0x1F80.
* CR0.{TS, EM} are zero.

## Protocols:

These are basically device-class interfaces. Each protocol has a unique GUID. The OS can take a given protocol GUID and query each device to see if it supports the protocol via `HandleProtocol()` or `OpenProtocol()`. I.e: you can take say, the Serial IO Protocol, or network protocol, and query each device to see if it supports it. If the device supports the given protocol, it will return a handle to a Protocol Interface struct. Drivers can register their supported protocols via `InstallProtocolInterface()`.

Sec 2.4:
* Protocol functions shouldn't be called "at runtime" (prolly means past ExitBootServices()).
* Can be called at TPL (task prio level) <= TPL_NOTIFY.
* Not re-entrant or MP safe.

### Protocols of interest:

* EFI Loaded Image Protocol: Provides information on the image.
* EFI Loaded Image Device Path Protocol: Specifies the device path that was used when a PE/COFF image was loaded through the EFI Boot Service Load-Image().
* EFI_SIMPLE_TEXT_INPUT_PROTOCOL: Protocol interfaces for devices that support simple console style text input.
* EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL: Protocol interfaces for devices that support console style text displaying.
* EFI_SERIAL_IO_PROTOCOL: Protocol interfaces for devices that support serial character transfer.
* EFI_DECOMPRESS_PROTOCOL: Protocol interfaces to decompress an image that was compressed using the EFI Compression Algorithm.
* EFI_GRAPHICS_OUTPUT_PROTOCOL: Protocol interfaces for devices that support graphics output.
* EFI_EDID_DISCOVERED_PROTOCOL: Contains the EDID information retrieved from a video output device.
* EFI_EDID_ACTIVE_PROTOCOL: Contains the EDID information for an active video output device.
* EFI_EDID_OVERRIDE_PROTOCOL: Produced by the platform to allow the platform to provide EDID information to the producer of the Graphics Output protocol.

### Service Protocols:

These are basically used to implement multiplexing subsystems like network protocol stacks.

## EFI's Required Implementation Elements:

The folloowing minimum set of elements are required for a compliant UEFI implementation (sec 2.6.1):
* EFI System Table
* EFI_BOOT_SERVICES
* EFI_RUNTIME_SERVICES
* EFI Loaded Image Protocol
* EFI Loaded Image Device Path Protocol
* EFI Device Path Protocol
* EFI_DECOMPRESS_PROTOCOL
* EFI_DEVICE_PATH_UTILITIES_PROTOCOL

## Platform Specific elements (sec 2.6.2):

Particular platform-specific elements of interest:

* If a platform includes console devices, then the following must be implemented: EFI_SIMPLE_TEXT_INPUT_PROTOCOL, EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL, and EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL.
* If a platform includes graphical console devices, then EFI_GRAPHICS_OUTPUT_PROTOCOL, EFI_EDID_DISCOVERED_PROTOCOL, and EFI_EDID_ACTIVE_PROTOCOL must be implemented. In order to support the EFI_GRAPHICS_OUTPUT_PROTOCOL, a platform must contain a driver to consume EFI_GRAPHICS_OUTPUT_PROTOCOL and produce EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL even if the EFI_GRAPHICS_OUTPUT_PROTOCOL is produced by an external driver.
* If a platform includes a byte-stream device such as a UART, then the EFI_SERIAL_IO_PROTOCOL must be implemented.
* If a platform includes PCI bus support, then the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL, the EFI PCI I/O Protocol, must be implemented.
* If a platform includes an I/O subsystem that utilizes SCSI, then the EFI_EXT_SCSI_PASS_THRU_PROTOCOL must be implemented.
* If a platform includes the ability to authenticate UEFI images and the platform potentially supports more than one OS loader, it must support the methods described in Secure Boot and Driver Signing and the authenticated UEFI variables described in Variable Services.
* If the platform supports UEFI secure boot as described in Secure Boot and Driver Signing, the platform must provide the PKCS verification functions described in PKCS7 Verify Protocol .
* If a platform includes the ability to register for notifications when a call to ResetSystem is called, then the EFI_RESET_NOTIFICATION_PROTOCOL must be implemented.

#### Platforms whose Runtime Services disappear after ExitBootServices():

> If a platform cannot support calls defined in EFI_RUNTIME_SERVICES after ExitBootServices() is called, thatplatform is permitted to provide implementations of those runtime services that return EFI_UNSUPPORTED when invoked at runtime. On such systems, an `EFI_RT_PROPERTIES_TABLE` configuration table should be published describing which runtime services are supported at runtime.

#### Load Option Variables (NVRAM):

* If a platform permits the installation of Load Option Variables, (Boot####, or Driver####, or SysPrep####), the platform must support and recognize all defined values for Attributes within the variable and report these capabilities in BootOptionSupport. If a platform supports installation of Load Option Variables of type Driver####, all installed Driver#### variables must be processed and the indicated driver loaded and initialized during every boot. And all installed SysPrep#### options must be processed prior to processing Boot#### options.

#### RISC-V:

RISC-V platform firmware must implement the RISCV_EFI_BOOT_PROTOCOL. OS loaders should use the RISCV_EFI_BOOT_PROTOCOL.GetBootHartId() to obtain the boot hart ID. The boot hart ID information provided by either SMBIOS or Device Tree is to be ignored by OS loaders. See “Links to UEFI Specification-Related Document” on https://uefi.org/uefi under the heading “RISC-V EFI Boot Protocol.”

## Digitally signed drivers:

Section 2.6.3:
> If a driver is digitally signed, it must embed the digital signature in the PE/COFF image as described in Embedded Signatures .

## Boot Manager:

Use `SetVariable()` to set "Load Option" variables. Each load option is stored in a var whose name is of the form (sec 3.1.1):
> Boot####, Driver####, SysPrep####, OsRecovery#### or PlatformRecovery####

PlatformRecovery####: holds info about some recovery image.
> The contents of PlatformRecovery#### represent the final recovery options the firmware would have attempted had recovery been initiated during the current boot, and need not include entries to reflect contingencies such as significant hardware reconfiguration, or entries corresponding to specific hardware that the firmware is not yet aware of.

The load options are ordered in 2 ordering list vars: "DriverOrder" and "BootOrder". DriverOrder must be processed before BootOrder.

### Recovery sequences:

* If `OsIndications.EFI_OS_INDICATIONS_START_OS_RECOVERY` bit is set, then BootMgr will initiate "OS-defined recovery" (sec 3.1.2) instead of the normal boot process.
* If `OsIndications.EFI_OS_INDICATIONS_START_PLATFORM_RECOVERY` bit is set, then BootMgr will initiate "platform-defined recovery" (sec 3.1.2) instead of the normal boot process.

If either recovery sequence is executed, then *both* bits are unset, and on the next boot *only*, BootMgr will boot into the "BootNext" option (and delete BootNext).

### Short-form device paths:

* USB short forms by WWID or Device Class ID.
* "Hard Drive Media Device Path": Allows the disk's physical location to chance, yet still be successfully booted from. Consists of a (GUID OR Signature) + a partition number.
* "File Path Media Device Path": Gives a file path, and then the BootMgr will enumerate all removable media devices __followed by__ all fixed media devices. It will then boot into the device that contains the given file.
* "URI Device Path": I didn't quite understand but it seems like BootMgr will literally try every possible device that exposes a "LoadFile" protocol instance.

### BootOrder vs OsRecoveryOrder+PlatformRecoveryOrder:

BoorOrder contains the hex indexes of Boot#### vars, in sequential order.
OSRecoveryOrder and PlatformRecoveryOrder contain the VendorGuids of vendors in sequential order. The BootMgr then looks for all the vars with a given VendorGuid and tries them in order, per VendorGuid.

### Default FileNames for bootable applications:

When no file path is provided, BootMgr will look for a file with the following name in the following path:
> \EFI\BOOT\BOOT<cpu_architecture>.efi

## UEFI Vars:

### Architecturally defined global vars:

All arch-defined global vars have the GUID "8be4df61-93ca-11d2-aa0d-00e098032b8c". All non-arch defined vars must use a vendor-defined GUID given by "VendorGuid" (sec 3.3) (spec italicized it like a var name but I don't see it in /efivars on my machine).

### Global vars:

These arch-def global vars are of interest:
* BootNext: The boot option for the next boot only.
* BootOptionSupport: The types of boot options supported by the boot manager. Should be treated as read-only.
* ConIn: The device path of the default input console. `ConInDev` lists all input consoles.
* ConOut: The device path of the default output console. `ConOutDev` lists all output consoles.
* ErrOut: The device path of the default error console. `ErrOutDev` lists all error consoles.
* SecureBoot: Whether or not the system is in secure boot mode.
* OsIndications: Allows the OS to request the firmware to enable certain features and to take certain actions.
* OsIndicationsSupported: Allows the firmware to indicate supported features and actions to the OS.

In particular it seems like the OS can just read ConOut instead of searching for a SIMPLE_TEXT_OUTPUT_PROTOCOL producer.

### UEFI Variables:

* UEFI variables are used to store configuration data for UEFI applications and drivers.
* UEFI variables are stored in the NVRAM (non-volatile RAM) and are persistent across system reboots.
* UEFI variables are identified by a unique GUID (Global Unique Identifier).

## UEFI Image handles:

Image handles passed to UEFI applications have the following protocols pre-filled in by the firmware:
> EFI_LOADED_IMAGE_PROTOCOL, EFI_LOADED_IMAGE_DEVICE_PATH_PROTOCOL.

Driver apps are expected to register and produce the following protocols:
> EFI_DRIVER_BINDING_PROTOCOL.

## UEFI Configuration Tables:

Tables of note (sec 4.6):
* ACPI 1.0 and 2.0 tables (phys only, no fixup).
* SMBIOS, SMBIOS3 tables (phys only, no fixup).
* MPS table (most likely Intel MP Spec table) (phys only, no fixup).
* Flattened Device Tree (FDT) (phys only, no fixup).
* EFI_RT_PROPERTIES_TABLE (phys only, no fixup): This table has a bitmask of supported EFI Runtime services. Can be ignored because unsupported services must still be implemented to return EFI_UNSUPPORTED.
* EFI_MEMORY_ATTRIBUTES_TABLE (phys only, no fixup): Tells the kernel how to page-attribute protect the EFI memory regions.
* EFI_CONFORMANCE_PROFILE_TABLE (phys only, no fixup). May be important if it works like the MP spec default profiles.



## Caveats:

* Remember to call SetVirtualAddressMap() when enabling paging.
* All supported UEFI arches must use litte-endian memory model.
* UEFI images use PE32+ format - has special signature at start of file.
