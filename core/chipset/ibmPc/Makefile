.PHONY=
DELIVERABLE=../../chipsetCore.a
OBJFILES=regionMap.o terminal.o \
	firmwareStream.o pic.o chipsetSupportPackage.o findTables.o \
	memoryMod.o terminalMod.o chipsetPackage.o ibmPc_modules.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean: fonyphile
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

