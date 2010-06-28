.PHONY=
DELIVERABLE=../../__kcxxlib.a
OBJFILES=new.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean: aclean dirclean

aclean: fonyphile

dirclean: fonyphile
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

