.PHONY=
DELIVERABLE=../../drivers.a
OBJFILES=zramdisk.o zrootdev.o zrootdev_init.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean: fonyphile
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

