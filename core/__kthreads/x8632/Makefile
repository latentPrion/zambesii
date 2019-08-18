.PHONY=
DELIVERABLE=../../__karchThreads.a
# We DO NOT link crt0.o in here. It is built, but not linked.
OBJFILES=main.o __kcpuPowerOnEntry.o \
	__kcpuPowerOnMain.o

# Build crt0.o, but DON'T allow it to be added to the .a archive. It will be
# linked later. We only link it at the very last linking step. Linking it now
# will cause it to be linked twice, and therefore cause
# multiple-symbol-definition linker errors.
all: Makefile $(DELIVERABLE) crt1.o crt0.o crti.o crtn.o

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

main.o: CXXFLAGS+=-Wno-variadic-macros

crt1.o: crt0.o
	cp crt0.o crt1.o

crt0.o: crt0Tmp.s
crt0Tmp.s: crt0.S
	$(CPP) $(CPPFLAGS) crt0.S >$@

__kcpuPowerOnEntry.o: __kcpuPowerOnEntryTmp.s
__kcpuPowerOnEntryTmp.s: __kcpuPowerOnEntry.S
	$(CPP) $(CPPFLAGS) __kcpuPowerOnEntry.S >$@

clean: fonyphile
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

