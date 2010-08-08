.PHONY=
DELIVERABLE=../../firmwareCore.a
OBJFILES=firmwareStream.o \
	debug.o decode.o fpu.o ops.o ops2.o prim_ops.o sys.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean: fonyphile
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

