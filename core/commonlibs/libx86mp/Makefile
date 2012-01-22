.PHONY=
DELIVERABLE=../../libx86mp.a
OBJFILES=lapic.o mpTables.o mpDefaultTables.o ipi.o libIoApic.o ioApic.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean: fonyphile
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

