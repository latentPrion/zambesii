.PHONY=
DELIVERABLE=
OBJFILES=

all: Makefile langSubdir

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

langSubdir:
	cd $(ZLANG); make

clean:
	rm -rf *.o *.s
	cd $(ZLANG); make clean

fonyphile:
	rm -f clean aclean dirclean

