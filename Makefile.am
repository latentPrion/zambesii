#
# Top level Makefile uses ZBZ toolchain settings by default.
#

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = __kcore #libraries #programs resources drivers
.PHONY: iso9660 imm __kexec programs libraries resources drivers isodir

CLEANFILES = zambesii.iso

#
# The kernel core file produced here must be relocatable. We have to link it
# with the Endo Kernel FS fragments later on in the kernel build process.
#
#$(GOAL): $(EXECOBJS)
#	$(LD) $(LDFLAGS) -nostartfiles -r -o $(GOAL) \
#		-Xlinker -\( $(EXECOBJS) -Xlinker -\)
#
#zambesii.zxe: __kcore drivers libraries resources programs
#	$(HOST_LD) $(HOST_LDFLAGS) \
#		-T $(srcdir)/core/__klinkScript.ld -o $@ \
#		$(builddir)/core/core.o

noinst_PROGRAMS = zambesii.zxe
zambesii_zxe_LDFLAGS=-nodefaultlibs \
	-T __kcore/platform/$(ZARCH)-$(ZCHIPSET)/__klinkScript.ld \
	-B__kcore/__kstdlib/__klibc/$(ZARCH)
zambesii_zxe_LDADD=__kcore/lib__kcore.a
zambesii_zxe_SOURCES=__kendoFs.S force_cpp_link.cpp

#
# Packaging targets
#
iso9660: zambesii.iso
zambesii.iso: __kexec isodir
	cp -f zambesii.zxe iso/zambesii
	$(ISOPROG) -R -b boot/grub/stage2_eltorito -no-emul-boot \
	-boot-load-size 4 -boot-info-table -V zambesii -o zambesii.iso iso

# Create the ISO directory structure
isodir: iso/zambesii
iso/zambesii:
	tar -xzf scripts/iso.tar.gz -C $(builddir)
	mkdir -p iso/zambesii

# For personal use
imm: $(BOOTDIR)/zambesii.zxe $(BOOTDIR)/zambesii.iso
$(BOOTDIR)/zambesii.zxe: zambesii.zxe
	cp -f zambesii.zxe $(BOOTDIR)/zambesii.zxe
$(BOOTDIR)/zambesii.iso: zambesii.iso
	cp -f zambesii.iso $(BOOTDIR)/zambesii.iso

__kexec: zambesii.zxe
programs:
#	make programs/programs.ekf
libraries:
#	make libraries/libraries.ekf
resources:
#	make resources/resources.ekf
drivers:
#	make drivers/drivers.ekf
