.PHONY=
DELIVERABLE=../../../floodplainn.a
OBJFILES=deviceTree.o floodplainn.o fvfs.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean:
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

