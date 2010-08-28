.PHONY=
DELIVERABLE=../../chipsetCore.a
OBJFILES=regionMap.o terminal.o \
	firmwareStream.o pic.o chipsetSupportPackage.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean: fonyphile
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

