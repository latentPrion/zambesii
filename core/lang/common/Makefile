.PHONY=
DELIVERABLE=../../lang.a
OBJFILES=mm.o lang.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean: aclean dirclean

aclean:

dirclean:
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

