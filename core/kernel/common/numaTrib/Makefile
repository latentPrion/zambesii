.PHONY=
DELIVERABLE=../../../numaTrib.a
OBJFILES=numaTrib.o numaStream.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

archSubdir:
	cd ../../$(ZARCH)/numaTrib; make

clean:
	rm -rf *.o *.s
	cd ../../$(ZARCH)/numaTrib; make dirclean

fonyphile:
	rm -f clean aclean dirclean

