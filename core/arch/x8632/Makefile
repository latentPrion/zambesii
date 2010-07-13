.PHONY=
DELIVERABLE=../../archCore.a
OBJFILES=wPRanger_mapInc.o wPRanger_mapNoInc.o wPRanger_remapInc.o \
	wPRanger_remapNoInc.o wPRanger_setAttributes.o wPRanger_lookup.o \
	wPRanger_unmap.o wPRanger_getLevelRanges.o wPRanger_accessors.o \
	wPRanger_convertFlags.o \
	tlbControl.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean: fonyphile
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

