.PHONY=
DELIVERABLE=../../archCore.a
_OBJFILES=wPRanger_mapInc.o wPRanger_mapNoInc.o wPRanger_remapInc.o \
	wPRanger_remapNoInc.o wPRanger_setAttributes.o wPRanger_lookup.o \
	wPRanger_unmap.o wPRanger_getLevelRanges.o wPRanger_accessors.o \
	wPRanger_convertFlags.o \
	tlbControl.o \
	gdt.o idt.o vectorHandlers.o setjmp.o string.o __kexceptionTable.o \
	exceptions.o x8632_pageFault.o x8632_generalProtectionFault.o \
	string8.o string16.o \
	taskContext.o cpuid.o msr.o cpuEnumerationIntel.o cpuEnumerationAmd.o \
	debug.o x8632_debugException.o x8632_breakpointException.o

THISDIR:=$(patsubst %/,%,$(dir $(abspath $(lastword ${MAKEFILE_LIST}))))
OBJFILES:=$(addprefix ${THISDIR}/, ${_OBJFILES})

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

${THISDIR}/vectorHandlers.o: vectorHandlersTmp.s
vectorHandlersTmp.s: vectorHandlers.S
	$(CPP) $(CPPFLAGS) vectorHandlers.S >$@

${THISDIR}/setjmp.o: setjmpTmp.s
setjmpTmp.s: setjmp.S
	$(CPP) $(CPPFLAGS) setjmp.S >$@

${THISDIR}/cpuid.o: cpuidTmp.s
cpuidTmp.s: cpuid.S
	$(CPP) $(CPPFLAGS) cpuid.S >$@

${THISDIR}/msr.o: msrTmp.s
msrTmp.s: msr.S
	$(CPP) $(CPPFLAGS) msr.S >$@

clean: fonyphile
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

