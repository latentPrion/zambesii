.PHONY=
DELIVERABLE=../../libacpi.a
OBJFILES=rsdp.o rxsdt.o madt.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean: fonyphile
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

