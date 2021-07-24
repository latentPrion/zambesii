.PHONY=
DELIVERABLE=../__kclasses.a
_OBJFILES=memBmp.o bitmap.o allocTable.o pageTableCache.o slamCache.o \
	debugPipe.o debugBuffer.o memReservoir.o heap.o cachePool.o \
	hardwareIdList.o singleWaiterQueue.o \
	test_memBmp.o

THISDIR:=$(patsubst %/,%,$(dir $(abspath $(lastword ${MAKEFILE_LIST}))))
OBJFILES:=$(addprefix ${THISDIR}/, ${_OBJFILES})

all: Makefile $(DELIVERABLE)

${THISDIR}/memReservoir.o: CXXFLAGS+=-Wno-frame-address
${THISDIR}/test_memBmp.o: CXXFLAGS+=-Wno-variadic-macros

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean: fonyphile
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

