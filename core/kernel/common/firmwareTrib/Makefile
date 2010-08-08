.PHONY=
DELIVERABLE=../../../firmwareTrib.a
OBJFILES=rivMemoryApi.o rivIoApi.o rivLockApi.o firmwareTrib.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean:
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

