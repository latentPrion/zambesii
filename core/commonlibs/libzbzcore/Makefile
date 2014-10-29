.PHONY=
DELIVERABLE=../../libzbzcore.a
OBJFILES=main.o distributaryPath.o driverPath.o devicePath.o layout.o \
	libzudi.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean: fonyphile
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

