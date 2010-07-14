.PHONY=
DELIVERABLE=
OBJFILES=

all: Makefile platformSubdir

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

platformSubdir:
	cd $(ZARCH)-$(ZCHIPSET); make

clean:
	rm -rf *.o *.s
	cd $(ZARCH)-$(ZCHIPSET); make clean

fonyphile:
	rm -f clean

