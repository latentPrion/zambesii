.PHONY=
DELIVERABLE=../../../cpuTrib.a
_OBJFILES=cpuTrib.o cpuStream.o chipsetSmpMode.o interCpuMessager.o

THISDIR:=$(patsubst %/,%,$(dir $(abspath $(lastword ${MAKEFILE_LIST}))))
OBJFILES:=$(addprefix ${THISDIR}/, ${_OBJFILES})

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean:
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

