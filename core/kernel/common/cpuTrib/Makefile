.PHONY=
DELIVERABLE=../../../cpuTrib.a
OBJFILES=cpuTrib.o cpuStream.o chipsetSmpMode.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean:
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

