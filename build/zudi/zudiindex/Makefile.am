ZUDIINDEX_TOP_SRCDIR::=$(top_srcdir)/../../programs/zudi
ZUDIINDEX_CURR_SRCDIR::=$(ZUDIINDEX_TOP_SRCDIR)/zudiindex

BUILT_SOURCES=zudipropsc.h
noinst_PROGRAMS=zudiindex
zudiindex_SOURCES=zudiindex.cpp parse.cpp index.cpp zudipropsc.h

%.cpp: $(ZUDIINDEX_CURR_SRCDIR)/%.cpp
	$(LN_S) $< $@

%.h: $(ZUDIINDEX_CURR_SRCDIR)/%.h
	$(LN_S) $< $@

noinst_SCRIPTS=zudiindex-addall

zudiindex-addall: $(ZUDIINDEX_CURR_SRCDIR)/zudiindex-addall
	$(LN_S) $< $@
	$(CHMOD) +x $@

CLEANFILES = zudiindex-addall $(builddir)/*.cpp $(builddir)/*.h
