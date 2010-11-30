.PHONY=
DELIVERABLE=../__kclasses.a
OBJFILES=memBmp.o bitmap.o allocTable.o pageTableCache.o slamCache.o \
	debugPipe.o debugBuffer.o memReservoir.o memoryBog.o cachePool.o \
	prioQueue.o hardwareIdList.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean: fonyphile
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

