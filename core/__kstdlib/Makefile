.PHONY=
DELIVERABLE=../__kstdlib.a
OBJFILES=utf8.o new.o

all: Makefile $(DELIVERABLE)

../__kstdlib.a: $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean: fonyphile
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

