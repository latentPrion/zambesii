.PHONY=
DELIVERABLE=../../../memoryTrib.a
OBJFILES=memoryStream.o memoryTrib.o vaddrSpaceStream.o rawMemAlloc.o \
	__kspaceMemAlloc.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean: aclean dirclean

aclean:

dirclean:
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

