.PHONY=
DELIVERABLE=../../firmwareCore.a
OBJFILES=firmwareStream.o \
	
	x86EmuAuxFuncs.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean: fonyphile
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

