.PHONY=
DELIVERABLE=../../firmwareCore.a
OBJFILES=firmwareStream.o x86emu.o x86emu_util.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean: fonyphile
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

