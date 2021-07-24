.PHONY=
DELIVERABLE=../../libzbzcore.a
_OBJFILES=main.o distributaryPath.o driverPath.o devicePath.o layout.o \
	libzudi.o libzudi_buf.o libzudi_dma.o

THISDIR:=$(patsubst %/,%,$(dir $(abspath $(lastword ${MAKEFILE_LIST}))))
OBJFILES:=$(addprefix ${THISDIR}/, ${_OBJFILES})

all: Makefile $(DELIVERABLE)

${THISDIR}/%.o: CXXFLAGS+=-Wno-variadic-macros

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean: fonyphile
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

