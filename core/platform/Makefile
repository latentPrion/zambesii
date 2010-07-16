.PHONY=
DELIVERABLE=../platform.a
OBJFILES=foo.o

all: Makefile $(DELIVERABLE) platformSubdir

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

platformSubdir:
	cd $(ZARCH)-$(ZCHIPSET); make

clean:
	rm -rf *.o *.s
	cd $(ZARCH)-$(ZCHIPSET); make clean

fonyphile:
	rm -f clean

