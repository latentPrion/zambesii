.PHONY=
DELIVERABLE=../../platformCore.a
OBJFILES=__kpagingLevel0Tables.o __kpagingLevel1Tables.o

all: Makefile $(DELIVERABLE) ../../__klinkScript.ld

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

__kpagingLevel0Tables.o: __kpagingLevel0TablesTmp.s
__kpagingLevel0TablesTmp.s: __kpagingLevel0Tables.S
	$(CPP) $(CPPFLAGS) __kpagingLevel0Tables.S >$@

__kpagingLevel1Tables.o: __kpagingLevel1TablesTmp.s
__kpagingLevel1TablesTmp.s: __kpagingLevel1Tables.S
	$(CPP) $(CPPFLAGS) __kpagingLevel1Tables.S >$@

../../__klinkScript.ld: __klinkScriptTmp.s
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	@echo Generating platform specific linker script.
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	grep -v '^[[:space:]]*#' __klinkScriptTmp.s > $@

__klinkScriptTmp.s: __klinkScript.S
	$(CPP) $(CPPFLAGS) __klinkScript.S >$@

clean:
	rm -rf *.o *.s

fonyphile:
	rm -f clean

