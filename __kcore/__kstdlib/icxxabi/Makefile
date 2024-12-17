.PHONY=
DELIVERABLE=../../__kcxxabi.a
_OBJFILES=dso.o callConstructors.o

THISDIR:=$(patsubst %/,%,$(dir $(abspath $(lastword ${MAKEFILE_LIST}))))
OBJFILES:=$(addprefix ${THISDIR}/, ${_OBJFILES})

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean: fonyphile
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

