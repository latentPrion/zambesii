.PHONY=
DELIVERABLE=../../__karchThreads.a
OBJFILES= crt0.o main.o __kcpuPowerOnEntry.o \
	__kcpuPowerOnMain.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

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

