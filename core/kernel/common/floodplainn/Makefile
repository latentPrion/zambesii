.PHONY=
DELIVERABLE=../../../floodplainn.a
OBJFILES=device.o fvfs.o floodplainn_zudiIndexServer_frontEnd.o \
	floodplainn_zudiIndexServer_backEnd.o floodplainn.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean:
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

