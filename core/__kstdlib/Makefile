.PHONY=
DELIVERABLE=../__kstdlib.a
_OBJFILES=new.o malloc.o __kmath.o test___kbitManipulation.o debug.o

THISDIR:=$(patsubst %/,%,$(dir $(abspath $(lastword ${MAKEFILE_LIST}))))
OBJFILES:=$(addprefix ${THISDIR}/, ${_OBJFILES})

all: Makefile $(DELIVERABLE)

../__kstdlib.a: $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

${THISDIR}/test___kbitManipulation.o: CXXFLAGS+=-Wno-variadic-macros

clean: fonyphile
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean
