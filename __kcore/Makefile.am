# __kcxxabi will soon be transformed into a compiler specific directory which
# will auto-build the appropriate C++ runtime for the current compiler.

SUBDIRS = __kstdlib __kthreads platform chipset firmware arch \
	kernel commonlibs __kclasses

BUILT_SOURCES = $(ZUDI_INDEX_FILES)

__KCORE_LIBS = \
	__kthreads/$(ZARCH)/lib__kthreadsCore.a \
	__kthreads/lib__kthreads.a \
	chipset/libchipset.a chipset/$(ZCHIPSET)/libchipsetCore.a \
	firmware/$(ZFIRMWARE)/libfirmwareCore.a \
	arch/$(ZARCH)/libarchCore.a \
	platform/$(ZARCH)-$(ZCHIPSET)/libplatformCore.a \
	kernel/common/libkernel.a kernel/$(ZARCH)/libkernelCore.a
# Tributaries
__KCORE_LIBS += kernel/common/cpuTrib/libcpuTrib.a \
	kernel/common/debugTrib/libdebugTrib.a \
	kernel/common/distributaryTrib/libdistributaryTrib.a \
	kernel/common/execTrib/libexecTrib.a kernel/common/floodplainn/libfloodplainn.a \
	kernel/common/interruptTrib/libinterruptTrib.a kernel/common/memoryTrib/libmemoryTrib.a \
	kernel/common/processTrib/libprocessTrib.a kernel/common/taskTrib/libtaskTrib.a \
	kernel/common/timerTrib/libtimerTrib.a kernel/common/vfsTrib/libvfsTrib.a
# Distributaries
__KCORE_LIBS += @DISTRIBUTARY_KERNEL_LIBS@
__KCORE_LIBS += @ZCHIPSET_COMMONLIBS_KERNEL_LIBS@ \
	commonlibs/libcommonlibs.a commonlibs/drivers/libdrivers.a \
	commonlibs/metalanguages/libmetalanguages.a
# Stdlibs
__KCORE_LIBS += __kclasses/lib__kclasses.a \
	__kstdlib/__klibcxx/lib__klibcxx.a \
	__kstdlib/__klibc/lib__klibc.a \
	__kstdlib/lib__kstdlib.a

noinst_LIBRARIES=lib__kcore.a
#lib__kcore_a_LIBADD=lib__kcore_prelink.a
lib__kcore_a_LIBADD=$(__KCORE_LIBS)
lib__kcore_a_SOURCES=udi_index.S

lib__kcore_prelink.a: $(__KCORE_LIBS)
#	As documentation: -Xlinker and -Wl both pass
#	arguments to the linker. However, -Xlinker is used to
#	pass arguments that are not recognized by the compiler,
#	and so the compiler should not try to interpret them.
	$(CXX) -r -o $@ -Wl,--start-group $^ -Wl,--end-group

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
