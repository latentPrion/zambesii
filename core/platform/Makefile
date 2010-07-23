.PHONY=
DELIVERABLE=../platform.a
OBJFILES=foo.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

platformSubdir:
	cd $(ZARCH)-$(ZCHIPSET); make

clean:
	rm -rf *.o *.s

fonyphile:
	rm -f clean

