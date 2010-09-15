.PHONY=
DELIVERABLE=../../../numaTrib.a
OBJFILES=numaTrib.o numaStream.o numaTrib_initialize2.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean:
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

