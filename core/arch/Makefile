.PHONY=
DELIVERABLE=../arch.a
OBJFILES=foo.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean: fonyphile
	rm -f *.o *.a

fonyphile:
	rm -f clean aclean dirclean

