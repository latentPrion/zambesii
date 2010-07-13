.PHONY=
DELIVERABLE=../../../cpuTrib.a
OBJFILES=cpuTrib.o cpuStream.o

all: Makefile $(DELIVERABLE) archSubdir

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

archSubdir:
	cd ../../$(ZARCH)/cpuTrib; make

clean:
	rm -rf *.o *.s
	cd ../../$(ZARCH)/cpuTrib; make clean


fonyphile:
	rm -f clean aclean dirclean

