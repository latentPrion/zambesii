# __kcxxabi will soon be transformed into a compiler specific directory which
# will auto-build the appropriate C++ runtime for the current compiler.

SUBDIRS = __kstdlib __kthreads platform chipset firmware arch \
	kernel commonlibs __kclasses

# Try not to change the order of these items. This is the order the linker
# runs through them in. If you change the order around, you'll begin to get
# undefined symbol complaints.
#
# As a rule, $(MAKE) sure to put object files with references to external symbols
# before object files without unresolved symbols. LD seems to run through all
# files resolving as it goes, so if it sees an unresolved symbol too late,
# it halts saying that the symbol cannot be resolved, even if the symbol is
# defined in one of the objects passed on the command line.
# EXECOBJS=__kthreads.a kernel.a archKernel.a firmwareCore.a \
#	platformCore.a \
#	memoryTrib.a interruptTrib.a execTrib.a distributaryTrib.a \
#	timerTrib.a cpuTrib.a processTrib.a vfsTrib.a taskTrib.a debugTrib.a \
#	floodplainn.a \
#	platformCore.a chipset.a chipsetCore.a archCore.a firmwareCore.a \
#	__kclasses.a __kstdlib.a __kcxxabi.a libx86mp.a libacpi.a \
#	commonlibs/libcommonlibs.a libzbzcore.a udi_index.o \
#	drivers.a metalanguages.a

BUILT_SOURCES = $(ZUDI_INDEX_FILES)

noinst_LIBRARIES=lib__kcore.a
lib__kcore_a_LIBADD=__kstdlib/__klibc/lib__klibc.a __kstdlib/__klibcxx/lib__klibcxx.a \
	__kstdlib/lib__kstdlib.a \
	__kthreads/lib__kthreads.a \
	chipset/libchipset.a chipset/$(ZCHIPSET)/libchipsetCore.a \
	firmware/$(ZFIRMWARE)/libfirmwareCore.a \
	arch/$(ZARCH)/libarchCore.a
	kernel/common/libkernel.a kernel/$(ARCH)/libkernelCore.a
# Tributaries
lib__kcore_a_LIBADD += kernel/common/cpuTrib/libcpuTrib.a \
	kernel/common/debugTrib/libdebugTrib.a
	kernel/common/distributaryTrib/libdistributaryTrib.a \
	kernel/common/execTrib/libexecTrib.a kernel/common/floodplainn/libfloodplainn.a \
	kernel/common/interruptTrib/libinterruptTrib.a kernel/common/memoryTrib/libmemoryTrib.a \
	kernel/common/processTrib/libprocessTrib.a kernel/common/taskTrib/libtaskTrib.a \
	kernel/common/timerTrib/libtimerTrib.a kernel/common/vfsTrib/libvfsTrib.a
# Distributaries
lib__kcore_a_LIBADD += commonlibs/libcommonlibs.a commonlibs/drivers/libdrivers.a \
	commonlibs/metalanguages/libmetalanguages.a \
	@ZCHIPSET_COMMONLIBS_KERNEL_LIBS@

lib__kcore_a_LIBADD += @DISTRIBUTARY_KERNEL_LIBS@
lib__kcore_a_LIBADD += __kclasses/lib__kclasses.a
lib__kcore_a_SOURCES=udi_index.S

ZUDI_INDEX_FILES = drivers.zudi-index data.zudi-index provisions.zudi-index \
	devices.zudi-index ranks.zudi-index strings.zudi-index
# noinst_DATA=$(ZUDI_INDEX_FILES)

udi_index.S: $(ZUDI_INDEX_FILES)

$(ZUDI_INDEX_FILES): $(ZUDIINDEX) $(ZUDIINDEX_ADDALL)
	@echo "Creating driver index."
	$(ZUDIINDEX) -c le -i .
	@echo "Adding chipset specific drivers to index."
	$(ZUDIINDEX_ADDALL) -drivers chipset/$(ZCHIPSET) .
	@echo "Adding chipset specific metalanguages to index."
	$(ZUDIINDEX_ADDALL) -metas chipset/$(ZCHIPSET) .
	@echo "Adding common metalanguages to index."
	$(ZUDIINDEX_ADDALL) -metas commonlibs/metalanguages .
	@echo "Adding common drivers to index."
	$(ZUDIINDEX_ADDALL) -drivers commonlibs/drivers .

CLEANFILES = $(ZUDI_INDEX_FILES)
