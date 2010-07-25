.PHONY=
DELIVERABLE=../../../firmwareTrib.a
OBJFILES=firmwareRivApi.o firmwareTrib.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean:
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

