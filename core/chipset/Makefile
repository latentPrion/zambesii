.PHONY=
DELIVERABLE=../chipset.a
OBJFILES=__kmemory.o

all: Makefile $(DELIVERABLE) chipsetSubdir

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

chipsetSubdir:
	cd $(ZCHIPSET); make

clean: fonyphile
	rm -rf *.o *.s
	cd $(ZCHIPSET); make clean

fonyphile:
	rm -f clean aclean dirclean

