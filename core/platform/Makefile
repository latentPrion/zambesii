.PHONY=
DELIVERABLE=../platform.a
OBJFILES=__kpagingLevel0Tables.o __kpagingLevel1Tables.o

all: Makefile $(DELIVERABLE) __klinkScript.ld

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

__klinkScript.ld: __klinkScriptTmp.s
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	@echo Generating platform specific linker script.
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	grep -v '^[[:space:]]*#' __klinkScriptTmp.s > $@

__klinkScriptTmp.s: __klinkScript.S */__klinkScript.ld
	$(CPP) -E $(CPPFLAGS) __klinkScript.S >$@
	

__kpagingLevel0Tables.o: __kpagingLevel0TablesTmp.s
__kpagingLevel0TablesTmp.s: __kpagingLevel0Tables.S */__kpagingLevel0Tables.S
	$(CPP) -E $(CPPFLAGS) __kpagingLevel0Tables.S >$@

__kpagingLevel1Hll.o: __kpagingLevel1Hll.cpp */__kpagingLevel1Hll.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ -c __kpagingLevel1Hll.cpp

__kpagingLevel1Tables.o: __kpagingLevel1TablesTmp.s
__kpagingLevel1TablesTmp.s: __kpagingLevel1Tables.S */__kpagingLevel1Tables.S
	$(CPP) -E $(CPPFLAGS) __kpagingLevel1Tables.S >$@

clean: aclean dirclean

aclean:
	rm -rf __klinkScript.ld

dirclean:
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

