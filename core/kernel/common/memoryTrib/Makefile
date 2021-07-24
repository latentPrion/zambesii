.PHONY=
DELIVERABLE=../../../memoryTrib.a
_OBJFILES=memoryStream.o memoryTrib.o vaddrSpaceStream.o rawMemAlloc.o \
	memoryTrib_numa.o memoryTrib_pmemInit.o

THISDIR:=$(patsubst %/,%,$(dir $(abspath $(lastword ${MAKEFILE_LIST}))))
OBJFILES:=$(addprefix ${THISDIR}/, ${_OBJFILES})

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean:
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

