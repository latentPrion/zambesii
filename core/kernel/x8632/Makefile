.PHONY=
DELIVERABLE=../../archKernel.a
OBJFILES=__kclassInstances.o cpuTrib.o interruptTrib.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean:
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

