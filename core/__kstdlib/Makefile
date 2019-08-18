.PHONY=
DELIVERABLE=../__kstdlib.a
OBJFILES=new.o malloc.o __kmath.o test___kbitManipulation.o

all: Makefile $(DELIVERABLE)

../__kstdlib.a: $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

test___kbitManipulation.o: CXXFLAGS+=-Wno-variadic-macros

clean: fonyphile
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean
