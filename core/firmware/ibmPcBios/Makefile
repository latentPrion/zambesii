.PHONY=
DELIVERABLE=../../firmwareCore.a
_OBJFILES=debug.o decode.o fpu.o ops.o ops2.o prim_ops.o sys.o \
	x86EmuAuxFuncs.o ibmPcBios_setRegs.o ibmPcBios_getRegs.o \
	ibmPcBios_coreFuncs.o

THISDIR:=$(patsubst %/,%,$(dir $(abspath $(lastword ${MAKEFILE_LIST}))))
OBJFILES:=$(addprefix ${THISDIR}/, ${_OBJFILES})

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean: fonyphile
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

