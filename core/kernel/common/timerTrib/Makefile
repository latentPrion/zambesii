.PHONY=
DELIVERABLE=../../../timerTrib.a
OBJFILES=timerTrib.o timerQueue.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean:
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

