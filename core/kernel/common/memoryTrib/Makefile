.PHONY=
DELIVERABLE=../../../memoryTrib.a
OBJFILES=memoryStream.o memoryTrib.o vaddrSpaceStream.o rawMemAlloc.o \
	memoryTrib_numa.o memoryTrib_pmemInit.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean:
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

