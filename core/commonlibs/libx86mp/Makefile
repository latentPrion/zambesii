.PHONY=
DELIVERABLE=../../libx86mp.a
OBJFILES=mpTables.o mpDefaultTables.o \
	lapic_core.o lapic_ipi.o lapic_lint.o \
	libIoApic.o ioApic_core.o ioApic_zkcm.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean: fonyphile
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

