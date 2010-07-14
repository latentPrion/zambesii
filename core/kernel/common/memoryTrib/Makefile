.PHONY=
DELIVERABLE=../../../memoryTrib.a
OBJFILES=memoryStream.o memoryTrib.o vaddrSpaceStream.o rawMemAlloc.o

all: Makefile $(DELIVERABLE) archSubdir

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

archSubdir:
	cd ../../$(ZARCH)/memoryTrib; make

clean:
	rm -rf *.o *.s
	cd ../../$(ZARCH)/memoryTrib; make clean

fonyphile:
	rm -f clean aclean dirclean

