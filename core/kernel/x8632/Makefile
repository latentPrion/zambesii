.PHONY=
DELIVERABLE=../../archKernel.a
_OBJFILES=__kclassInstances.o cpuTrib.o interruptTrib.o cpuStream.o \
	interCpuMessager.o cpuStream_powerManager.o vaddrSpace.o

THISDIR:=$(patsubst %/,%,$(dir $(abspath $(lastword ${MAKEFILE_LIST}))))
OBJFILES:=$(addprefix ${THISDIR}/, ${_OBJFILES})

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean:
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

