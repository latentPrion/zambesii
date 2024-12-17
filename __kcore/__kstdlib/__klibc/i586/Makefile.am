BUILT_SOURCES = crt1.S

noinst_LIBRARIES=libzbzcrt.a
libzbzcrt_a_SOURCES=crt1.S crt0.S crti.S crtn.S

crt1.S: crt0.S
	ln -sf $(srcdir)/crt0.S $@

CLEANFILES = crt1.S
