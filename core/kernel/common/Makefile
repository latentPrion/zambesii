.PHONY=
DELIVERABLE=../../kernel.a
OBJFILES=__kheaders.o __koptimizationHacks.o __kclassInstances.o \
	multipleReaderLock.o waitLock.o recursiveLock.o vSwamp.o panic.o \
	numaMemoryBank.o preConstruct.o __kprocess.o stream.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean: aclean dirclean

aclean:

dirclean:
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

