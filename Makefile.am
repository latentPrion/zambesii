ACLOCAL_AMFLAGS = -I m4

SUBDIRS = __kcore #libraries #programs resources drivers
.PHONY: iso9660 imm __kexec programs libraries resources drivers isodir

CLEANFILES = zambesii.iso
DISTCLEANFILES = iso

# We may be able to use the following form to manipulate words
# within the __KCORE_LIBS variable:
#	FOO = $(__KCORE_LIBS:%=__kcore/%)
__KCORE_LIBS = __kcore/lib__kcore.a \
	__kcore/__kthreads/$(ZARCH)/lib__kthreadsCore.a \
	__kcore/__kthreads/lib__kthreads.a \
	__kcore/chipset/libchipset.a __kcore/chipset/$(ZCHIPSET)/libchipsetCore.a \
	__kcore/firmware/$(ZFIRMWARE)/libfirmwareCore.a \
	__kcore/arch/$(ZARCH)/libarchCore.a \
	__kcore/platform/$(ZARCH)-$(ZCHIPSET)/libplatformCore.a \
	__kcore/kernel/common/libkernel.a __kcore/kernel/$(ZARCH)/libkernelCore.a
# Tributaries
__KCORE_LIBS += __kcore/kernel/common/cpuTrib/libcpuTrib.a \
	__kcore/kernel/common/debugTrib/libdebugTrib.a \
	__kcore/kernel/common/distributaryTrib/libdistributaryTrib.a \
	__kcore/kernel/common/execTrib/libexecTrib.a __kcore/kernel/common/floodplainn/libfloodplainn.a \
	__kcore/kernel/common/interruptTrib/libinterruptTrib.a __kcore/kernel/common/memoryTrib/libmemoryTrib.a \
	__kcore/kernel/common/processTrib/libprocessTrib.a __kcore/kernel/common/taskTrib/libtaskTrib.a \
	__kcore/kernel/common/timerTrib/libtimerTrib.a __kcore/kernel/common/vfsTrib/libvfsTrib.a
# Distributaries
__KCORE_LIBS += @DISTRIBUTARIES_KERNEL_LIBS@
# Commonlibs
__KCORE_LIBS += @ZCHIPSET_COMMONLIBS_KERNEL_LIBS@ \
	__kcore/commonlibs/libcommonlibs.a __kcore/commonlibs/drivers/libdrivers.a \
	__kcore/commonlibs/metalanguages/libmetalanguages.a \
	__kcore/commonlibs/libzbzcore/libzbzcore.a
# Stdlibs
__KCORE_LIBS += __kcore/__kclasses/lib__kclasses.a \
	__kcore/__kstdlib/__klibcxx/lib__klibcxx.a \
	__kcore/__kstdlib/__klibc/lib__klibc.a \
	__kcore/__kstdlib/lib__kstdlib.a \
	__kcore/__kstdlib/icxxabi/libicxxabi.a

zambesii_zxe_LDSCRIPT=__kcore/platform/$(ZARCH)-$(ZCHIPSET)/__klinkScript.ld
noinst_PROGRAMS=zambesii.zxe
zambesii_zxe_DEPENDENCIES=$(__KCORE_LIBS) $(zambesii_zxe_LDSCRIPT)
# As documentation: -Xlinker and -Wl both pass
# arguments to the linker. However, -Xlinker is used to
# pass arguments that are not recognized by the compiler,
# and so the compiler should not try to interpret them.
zambesii_zxe_LDFLAGS=-nodefaultlibs \
	-T $(zambesii_zxe_LDSCRIPT) \
	-B"__kcore/__kthreads/$(ZARCH)" \
	-Wl,--start-group $(__KCORE_LIBS) -Wl,--end-group
zambesii_zxe_SOURCES=force_cpp_link.cpp

#
# Packaging targets
#
iso9660: zambesii.iso
zambesii.iso: __kexec isodir
	$(CP) -f zambesii.zxe iso/zambesii
	$(ISOPROG) -R -b boot/grub/stage2_eltorito -no-emul-boot \
	-boot-load-size 4 -boot-info-table -V zambesii -o zambesii.iso iso

# Create the ISO directory structure
isodir: iso/zambesii
iso/zambesii:
	$(TAR) -xzf $(srcdir)/build/iso.tar.gz -C $(builddir)
	$(MKDIR_P) iso/zambesii

# For personal use
imm: $(BOOTDIR)/zambesii.zxe $(BOOTDIR)/zambesii.iso
$(BOOTDIR)/zambesii.zxe: zambesii.zxe
	$(CP) -f zambesii.zxe $(BOOTDIR)/zambesii.zxe
$(BOOTDIR)/zambesii.iso: zambesii.iso
	$(CP) -f zambesii.iso $(BOOTDIR)/zambesii.iso

__kexec: zambesii.zxe
programs:
#	make programs/programs.ekf
libraries:
#	make libraries/libraries.ekf
resources:
#	make resources/resources.ekf
drivers:
#	make drivers/drivers.ekf
