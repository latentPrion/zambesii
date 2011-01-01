.PHONY=
DELIVERABLE=../../chipsetCore.a
OBJFILES=regionMap.o pic.o findTables.o \
	memoryMod.o terminalMod.o chipsetPackage.o ibmPc_modules.o cpuMod.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean: fonyphile
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

