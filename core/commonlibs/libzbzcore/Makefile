.PHONY=
DELIVERABLE=../../libzbzcore.a
OBJFILES=main.o distributaryPath.o driverPath.o devicePath.o layout.o \
	libzudi.o libzudi_buf.o libzudi_dma.o

all: Makefile $(DELIVERABLE)

%.o: CXXFLAGS+=-Wno-variadic-macros

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean: fonyphile
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

