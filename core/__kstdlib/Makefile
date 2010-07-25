.PHONY=
DELIVERABLE=../__kstdlib.a
OBJFILES=foo.o

all: Makefile $(DELIVERABLE)

../__kstdlib.a: $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean: fonyphile
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

