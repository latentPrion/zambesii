.PHONY=
DELIVERABLE=../../kernel.a
OBJFILES=__kheaders.o __koptimizationHacks.o __kclassInstances.o \
	multipleReaderLock.o waitLock.o recursiveLock.o vSwamp.o panic.o \
	numaMemoryBank.o numaMemoryRange.o thread.o process.o numaCpuBank.o \
	messageStream.o zasyncStream.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean:
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

