.PHONY=
DELIVERABLE=../../../floodplainn.a
_OBJFILES=device.o fvfs.o zuiFrontEnd.o zuiBackEnd.o floodplainn.o \
	udiServiceCalls.o udi_channel.o floodplainnStream.o channel.o \
	zumFrontEnd.o zumBackEnd.o udi_dma.o zudi.o

THISDIR:=$(patsubst %/,%,$(dir $(abspath $(lastword ${MAKEFILE_LIST}))))
OBJFILES:=$(addprefix ${THISDIR}/, ${_OBJFILES})

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean:
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean
