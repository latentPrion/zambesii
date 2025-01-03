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

udi_index.S: zudi_index_updated

ZUDIINDEX_BUILD_DIR ::= $(top_builddir)/build/zudi/zudiindex
ZUDIINDEX ::= $(ZUDIINDEX_BUILD_DIR)/zudiindex
ZUDIINDEX_ADDALL ::= $(ZUDIINDEX_BUILD_DIR)/zudiindex-addall
$(ZUDI_INDEX_FILES): $(ZUDIINDEX) $(ZUDIINDEX_ADDALL)
zudi_index_updated: $(ZUDI_INDEX_FILES)
	@echo "Creating driver index."
	$(ZUDIINDEX) -c le -i .
	@echo "Adding chipset specific drivers to index."
	ZUDIINDEX_BIN=${ZUDIINDEX} $(ZUDIINDEX_ADDALL) -drivers $(srcdir)/chipset/$(ZCHIPSET) .
	@echo "Adding chipset specific metalanguages to index."
	ZUDIINDEX_BIN=${ZUDIINDEX} $(ZUDIINDEX_ADDALL) -metas $(srcdir)/chipset/$(ZCHIPSET) .
	@echo "Adding common metalanguages to index."
	ZUDIINDEX_BIN=${ZUDIINDEX} $(ZUDIINDEX_ADDALL) -metas $(srcdir)/commonlibs/metalanguages .
	@echo "Adding common drivers to index."
	ZUDIINDEX_BIN=${ZUDIINDEX} $(ZUDIINDEX_ADDALL) -drivers $(srcdir)/commonlibs/drivers .
	$(TOUCH) zudi_index_updated

CLEANFILES = $(ZUDI_INDEX_FILES)
