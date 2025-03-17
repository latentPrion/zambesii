ACLOCAL_AMFLAGS = -I m4

SUBDIRS = build __kcore #libraries #programs resources drivers
.PHONY: grub-iso grub2-iso imm imm-grub2 __kexec programs libraries resources drivers isodir-grub isodir-grub2

# Define output directories for ISO files
ISO_GRUB_DIR = $(builddir)/packaging/iso/grub
ISO_GRUB2_DIR = $(builddir)/packaging/iso/grub2

# Add packaging files to distribution
EXTRA_DIST = packaging/grub/menu.lst packaging/grub/iso/stage2_eltorito \
             packaging/grub2/grub.cfg packaging/grub2/iso/cdboot.img packaging/grub2/iso/grub2-i386-modules.img

CLEANFILES = $(ISO_GRUB_DIR)/zambesii.iso $(ISO_GRUB2_DIR)/zambesii.iso
DISTCLEANFILES = iso-grub iso-grub2 $(ISO_GRUB_DIR) $(ISO_GRUB2_DIR)

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

# Define packaging targets
ISO_TARGETS = $(ISO_GRUB_DIR)/zambesii.iso $(ISO_GRUB2_DIR)/zambesii.iso

# Main targets
# Let Automake handle 'all' target normally

# GRUB1 ISO target
grub-iso: $(ISO_GRUB_DIR)/zambesii.iso

$(ISO_GRUB_DIR)/zambesii.iso: __kexec isodir-grub
	$(MKDIR_P) $(ISO_GRUB_DIR)
	$(CP) -f zambesii.zxe iso-grub/zambesii
	$(CP) -f $(srcdir)/packaging/grub/menu.lst iso-grub/boot/grub/menu.lst
	$(ISOPROG) -R -b boot/grub/stage2_eltorito -no-emul-boot \
	-boot-load-size 4 -boot-info-table -V zambesii -o $(ISO_GRUB_DIR)/zambesii.iso iso-grub

# Create the GRUB ISO directory structure
isodir-grub: iso-grub/zambesii
iso-grub/zambesii: __kexec
	$(MKDIR_P) iso-grub/boot/grub
	$(MKDIR_P) iso-grub/zambesii
	$(CP) -f $(srcdir)/packaging/grub/iso/stage2_eltorito iso-grub/boot/grub/

# GRUB2 ISO target
grub2-iso: $(ISO_GRUB2_DIR)/zambesii.iso

$(ISO_GRUB2_DIR)/zambesii.iso: __kexec isodir-grub2 packaging/grub2/iso/grub2-i386-modules.img
	$(MKDIR_P) $(ISO_GRUB2_DIR)
	$(CP) -f zambesii.zxe iso-grub2/zambesii
	$(CP) -f $(srcdir)/packaging/grub2/grub.cfg iso-grub2/boot/grub/grub.cfg
	cat $(srcdir)/packaging/grub2/iso/cdboot.img $(srcdir)/packaging/grub2/iso/grub2-i386-modules.img > iso-grub2/boot/grub/grub.img
	$(ISOPROG) -R -b boot/grub/grub.img -no-emul-boot \
	-boot-load-size 4 -boot-info-table -V zambesii -o $(ISO_GRUB2_DIR)/zambesii.iso iso-grub2

# Create the GRUB2 ISO directory structure
isodir-grub2: iso-grub2/zambesii
iso-grub2/zambesii: __kexec
	$(MKDIR_P) iso-grub2/boot/grub
	$(MKDIR_P) iso-grub2/zambesii

# For personal use
imm: $(MAKE_IMM_PATH)/zambesii.zxe $(MAKE_IMM_PATH)/zambesii.iso
imm-grub2: $(MAKE_IMM_PATH)/zambesii.zxe $(MAKE_IMM_PATH)/zambesii-grub2.iso

$(MAKE_IMM_PATH)/zambesii.zxe: __kexec
	$(CP) -f zambesii.zxe $(MAKE_IMM_PATH)/zambesii.zxe
$(MAKE_IMM_PATH)/zambesii.iso: $(ISO_GRUB_DIR)/zambesii.iso
	$(CP) -f $(ISO_GRUB_DIR)/zambesii.iso $(MAKE_IMM_PATH)/zambesii.iso
$(MAKE_IMM_PATH)/zambesii-grub2.iso: $(ISO_GRUB2_DIR)/zambesii.iso
	$(CP) -f $(ISO_GRUB2_DIR)/zambesii.iso $(MAKE_IMM_PATH)/zambesii-grub2.iso

# Kernel executable target (ensures kernel binary is built)
# all-recursive builds all subdirectory targets
# all builds top-level targets
__kexec: all-recursive all

# Additional targets
programs:
#	make programs/programs.ekf
libraries:
#	make libraries/libraries.ekf
resources:
#	make resources/resources.ekf
drivers:
#	make drivers/drivers.ekf
