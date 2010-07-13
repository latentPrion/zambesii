.PHONY=
DELIVERABLE=platformCore.a
OBJFILES=__kpagingLevel0Tables.o __kpagingLevel1Tables.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

__kpagingLevel0Tables.o: __kpagingLevel0TablesTmp.s
__kpagingLevel0TablesTmp.s:
	$(CPP) -E $(CPPFLAGS) __kpagingLevel0Tables.S >$@

__kpagingLevel1Tables.o: __kpagingLevel1TablesTmp.s
__kpagingLevel1TablesTmp.s:
	$(CPP) -E $(CPPFLAGS) __kpagingLevel1Tables.S >$@

clean:
	rm -rf *.o *.s

fonyphile:
	rm -f clean

