.PHONY=
DELIVERABLE=../../chipsetCore.a
OBJFILES=regionMap.o pic.o findTables.o \
	memoryDetection.o terminalMod.o zkcmCore.o ibmPc_modules.o \
	cpuDetection.o memoryAreas.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean: fonyphile
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

