.PHONY=
DELIVERABLE=../__kthreads.a
OBJFILES= main.o __kcpuPowerOn.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

main.o: CXXFLAGS+=-Wno-variadic-macros

clean: fonyphile
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

