.PHONY=
DELIVERABLE=../../kernel.a
OBJFILES=__kheaders.o __koptimizationHacks.o __kclassInstances.o \
	multipleReaderLock.o waitLock.o recursiveLock.o vSwamp.o panic.o \
	numaMemoryBank.o __kprocess.o stream.o

all: Makefile $(DELIVERABLE) archSubdir

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -S waitLock.cpp

archSubdir:
	cd ../$(ZARCH); make

clean:
	rm -rf *.o *.s
	cd ../$(ZARCH); make clean

fonyphile:
	rm -f clean aclean dirclean

