.PHONY=
DELIVERABLE=../../../numaTrib.a
OBJFILES=numaTrib.o numaStream.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean:
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

