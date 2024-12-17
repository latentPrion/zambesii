BUILT_SOURCES=crt1.S

noinst_LIBRARIES=lib__kthreadsCore.a
lib__kthreadsCore_a_SOURCES=main.cpp __kcpuPowerOnEntry.S __kcpuPowerOnMain.cpp \
    crt0.S crt1.S crti.S crtn.S

crt1.S: $(srcdir)/crt0.S
	$(LN_S) -f $< $@

main.$(OBJEXT): AM_CXXFLAGS += -Wno-variadic-macros

CLEANFILES=crt1.S
