.PHONY=
DELIVERABLE=../../chipsetCore.a
OBJFILES=memoryConfig.o memoryMap.o regionMap.o numaMap.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean: fonyphile
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

