ZUDIINDEX_ABS_TOP_SRCDIR=$(abs_top_srcdir)/../../programs/zudi
ZUDIINDEX_ABS_SRCDIR=$(ZUDIINDEX_ABS_TOP_SRCDIR)/zudiindex

BUILT_SOURCES=zudipropsc.h
noinst_PROGRAMS=zudiindex
zudiindex_SOURCES=zudiindex.cpp parse.cpp index.cpp zudipropsc.h

%.cpp: $(ZUDIINDEX_ABS_SRCDIR)/%.cpp
	$(LN_S) $< $@

%.h: $(ZUDIINDEX_ABS_SRCDIR)/%.h
	$(LN_S) $< $@

noinst_SCRIPTS=zudiindex-addall

zudiindex-addall: $(ZUDIINDEX_ABS_SRCDIR)/zudiindex-addall
	$(LN_S) $< $@
	$(CHMOD) +x $@

CLEANFILES = zudiindex-addall $(builddir)/*.cpp $(builddir)/*.h
