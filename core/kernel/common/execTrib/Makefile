.PHONY=
DELIVERABLE=../../../execTrib.a
OBJFILES=execTrib.o elf_base.o elfParser.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean:
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

