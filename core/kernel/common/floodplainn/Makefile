.PHONY=
DELIVERABLE=../../../floodplainn.a
OBJFILES=device.o floodplainn.o fvfs.o index.o driverIndexer.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean:
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

