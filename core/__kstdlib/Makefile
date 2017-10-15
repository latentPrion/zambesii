.PHONY=
DELIVERABLE=../__kstdlib.a
OBJFILES=new.o malloc.o __kmath.o test___kbitManipulation.o

all: Makefile $(DELIVERABLE)

../__kstdlib.a: $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean: fonyphile
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean
