# __kcxxabi will soon be transformed into a compiler specific directory which
# will auto-build the appropriate C++ runtime for the current compiler.

SUBDIRS = __kstdlib __kthreads platform chipset firmware arch \
	kernel commonlibs __kclasses

BUILT_SOURCES = $(ZUDI_INDEX_FILES)

noinst_LIBRARIES=lib__kcore.a
lib__kcore_a_SOURCES=udi_index.S

noinst_DATA=$(ZUDI_INDEX_FILES)
ZUDI_INDEX_FILES = drivers.zudi-index data.zudi-index provisions.zudi-index \
	devices.zudi-index ranks.zudi-index strings.zudi-index

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
