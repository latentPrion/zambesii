.PHONY=
DELIVERABLE=../__kthreads.a
OBJFILES= __korientationStack.o __korientationPreConstruct.o

all: Makefile $(DELIVERABLE) archSubdir

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

archSubdir:
	cd $(ZARCH); make

clean: fonyphile
	rm -rf *.o *.s
	cd $(ZARCH); make clean

fonyphile:
	rm -f clean aclean dirclean

