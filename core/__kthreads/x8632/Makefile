.PHONY=
DELIVERABLE=../../__karchThreads.a
OBJFILES= __korientationEntry.o __korientationMain.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

__korientationEntryTmp.s:
	$(CPP) -E $(CPPFLAGS) __korientationEntry.S >$@

__korientationEntry.o: __korientationEntryTmp.s

clean: fonyphile
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

