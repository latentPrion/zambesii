.PHONY=
DELIVERABLE=../../../numaTrib.a
OBJFILES=numaTrib.o numaStream.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean: aclean dirclean

aclean:

dirclean:
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

