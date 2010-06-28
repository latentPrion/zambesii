.PHONY=
DELIVERABLE=../__kthreads.a
OBJFILES=__korientationAsm.o __korientationEntry.o __korientationStack.o \
	__korientationPreConstruct.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

__korientationAsm.o: __korientationAsmTmp.s
__korientationAsmTmp.s: __korientationAsm.S */__korientationAsm.S
	$(CPP) -E $(CPPFLAGS) __korientationAsm.S >$@

__korientationEntry.o: __korientationEntry.cpp */__korientationEntry.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ -c __korientationEntry.cpp

clean: aclean dirclean

aclean: fonyphile

dirclean: fonyphile
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

