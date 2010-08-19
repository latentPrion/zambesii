.PHONY=
DELIVERABLE=../../archCore.a
OBJFILES=wPRanger_mapInc.o wPRanger_mapNoInc.o wPRanger_remapInc.o \
	wPRanger_remapNoInc.o wPRanger_setAttributes.o wPRanger_lookup.o \
	wPRanger_unmap.o wPRanger_getLevelRanges.o wPRanger_accessors.o \
	wPRanger_convertFlags.o \
	tlbControl.o \
	gdt.o idt.o vectorHandlers.o setjmp.o string.o __kexceptionTable.o \
	exceptions.o x8632_pageFault.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

vectorHandlers.o: vectorHandlersTmp.s
vectorHandlersTmp.s: vectorHandlers.S
	$(CPP) $(CPPFLAGS) vectorHandlers.S >$@

setjmp.o: setjmpTmp.s
setjmpTmp.s: setjmp.S
	$(CPP) $(CPPFLAGS) setjmp.S >$@

clean: fonyphile
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

