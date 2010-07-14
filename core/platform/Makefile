.PHONY=
DELIVERABLE=
OBJFILES=

all: Makefile $(DELIVERABLE) ../__klinkScript.ld platformSubdir

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

platformSubdir:
	cd $(ZARCH)-$(ZCHIPSET); make

../__klinkScript.ld: __klinkScriptTmp.s
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	@echo Generating platform specific linker script.
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	grep -v '^[[:space:]]*#' __klinkScriptTmp.s > $@

__klinkScriptTmp.s: __klinkScript.S */__klinkScript.ld
	$(CPP) $(CPPFLAGS) __klinkScript.S >$@

clean:
	rm -rf *.o *.s
	cd $(ZARCH)-$(ZCHIPSET); make clean

fonyphile:
	rm -f clean

