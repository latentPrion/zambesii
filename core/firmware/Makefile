# Remember that the shell var for here is $(ZFIRMWARE).

.PHONY=
DELIVERABLE=../firmware.a
OBJFILES=foo.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean: fonyphile
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

