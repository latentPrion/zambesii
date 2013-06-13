.PHONY=
DELIVERABLE=../../../distributaryTrib.a
OBJFILES=distributaryTrib.o distributaries.o distributaryTrib_dvfs.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean:
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

