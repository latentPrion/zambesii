.PHONY=
DELIVERABLE=../../archCore.a
OBJFILES=wPRanger_mapInc.o wPRanger_mapNoInc.o wPRanger_remapInc.o \
	wPRanger_remapNoInc.o wPRanger_setAttributes.o wPRanger_lookup.o \
	wPRanger_unmap.o wPRanger_getLevelRanges.o wPRanger_accessors.o \
	wPRanger_convertFlags.o \
	tlbControl.o \
	gdt.o idt.o vectorHandlers.o setjmp.o string.o __kexceptionTable.o \
	exceptions.o x8632_pageFault.o string8.o string16.o \
	taskContext.o cpuid.o msr.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

vectorHandlers.o: vectorHandlersTmp.s
vectorHandlersTmp.s: vectorHandlers.S
	$(CPP) $(CPPFLAGS) vectorHandlers.S >$@

setjmp.o: setjmpTmp.s
setjmpTmp.s: setjmp.S
	$(CPP) $(CPPFLAGS) setjmp.S >$@

cpuid.o: cpuidTmp.s
cpuidTmp.s: cpuid.S
	$(CPP) $(CPPFLAGS) cpuid.S >$@

msr.o: msrTmp.s
msrTmp.s: msr.S
	$(CPP) $(CPPFLAGS) msr.S >$@

clean: fonyphile
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

