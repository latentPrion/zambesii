ZUDIINDEX_ABS_TOP_SRCDIR=$(abs_top_srcdir)/../../programs/zudi
ZUDIINDEX_ABS_SRCDIR=$(ZUDIINDEX_ABS_TOP_SRCDIR)/zudiindex

noinst_PROGRAMS=zudiindex
zudiindex_SOURCES=$(ZUDIINDEX_ABS_SRCDIR)/zudiindex.cpp \
	$(ZUDIINDEX_ABS_SRCDIR)/parse.cpp \
	$(ZUDIINDEX_ABS_SRCDIR)/index.cpp

noinst_SCRIPTS=zudiindex-addall

zudiindex-addall: $(ZUDIINDEX_ABS_SRCDIR)/zudiindex-addall
	$(LN_S) $< $@
	$(CHMOD) +x $@

CLEANFILES = zudiindex-addall
