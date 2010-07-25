.PHONY=
DELIVERABLE=../../__karchThreads.a
OBJFILES= __korientationEntry.o __korientationMain.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

__korientationEntry.o: __korientationEntryTmp.s
__korientationEntryTmp.s: __korientationEntry.S
	$(CPP) $(CPPFLAGS) __korientationEntry.S >$@


clean: fonyphile
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

