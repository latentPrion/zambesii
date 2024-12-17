BUILT_SOURCES=__klinkScript.ld __klinkScriptTmp.s
noinst_LIBRARIES=libplatformCore.a
libplatformCore_a_SOURCES=__kpagingLevel0Tables.S __kpagingLevel1Tables.S

noinst_DATA=__klinkScript.ld

__klinkScript.ld: __klinkScriptTmp.s
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	@echo Generating platform specific linker script.
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	grep -v '^[[:space:]]*#' $< >$@

__klinkScriptTmp.s: __klinkScript.S
	$(CPP) $(AM_CPPFLAGS) $(AM_ASFLAGS) $(AM_CPPASFLAGS) $(CPPFLAGS) $(ASFLAGS) $(CPPASFLAGS) -I$(abs_top_builddir)/include -E >$@ $<

CLEANFILES=__klinkScript.ld __klinkScriptTmp.s
