.PHONY=
DELIVERABLE=../../chipsetCore.a
OBJFILES=regionMap.o terminal.o \
	firmwareStream.o pic.o chipsetSupportPackage.o ibmPc_mi_getNumaMap.o \
	ibmPc_mi_findSrat.o rs232.o findTables.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean: fonyphile
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

