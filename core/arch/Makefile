.PHONY=
DELIVERABLE=../arch.a
OBJFILES=foo.o

all: Makefile $(DELIVERABLE) archSubdir

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

archSubdir:
	cd $(ZARCH); make

clean: fonyphile
	cd $(ZARCH); make clean

fonyphile:
	rm -f clean aclean dirclean

