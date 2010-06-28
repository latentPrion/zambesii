.PHONY=
DELIVERABLE=../arch.a
OBJFILES=wPRanger_mapInc.o wPRanger_mapNoInc.o wPRanger_remapInc.o \
	wPRanger_remapNoInc.o wPRanger_setAttributes.o wPRanger_lookup.o \
	wPRanger_unmap.o wPRanger_getLevelRanges.o wPRanger_accessors.o \
	wPRanger_convertFlags.o \
	tlbControl.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

tlbControl.o: tlbControl.cpp */tlbControl.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ -c tlbControl.cpp

wPRanger_mapInc.o: wPRanger_mapInc.cpp */wPRanger_mapInc.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ -c wPRanger_mapInc.cpp

wPRanger_mapNoInc.o: wPRanger_mapNoInc.cpp */wPRanger_mapNoInc.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ -c wPRanger_mapNoInc.cpp

wPRanger_remapInc.o: wPRanger_remapInc.cpp */wPRanger_remapInc.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ -c wPRanger_remapInc.cpp

wPRanger_remapNoInc.o: wPRanger_remapNoInc.cpp */wPRanger_remapNoInc.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ -c wPRanger_remapNoInc.cpp

wPRanger_setAttributes.o: wPRanger_setAttributes.cpp \
 */wPRanger_setAttributes.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ -c wPRanger_setAttributes.cpp

wPRanger_lookup.o: wPRanger_lookup.cpp */wPRanger_lookup.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ -c wPRanger_lookup.cpp

wPRanger_unmap.o: wPRanger_unmap.cpp */wPRanger_unmap.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ -c wPRanger_unmap.cpp

wPRanger_getLevelRanges.o: wPRanger_getLevelRanges.cpp \
 */wPRanger_getLevelRanges.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ -c wPRanger_getLevelRanges.cpp

wPRanger_convertFlags.o: wPRanger_convertFlags.cpp */wPRanger_convertFlags.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ -c wPRanger_convertFlags.cpp

wPRanger_accessors.o: wPRanger_accessors.cpp */wPRanger_accessors.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ -c wPRanger_accessors.cpp

clean: aclean dirclean

aclean:

dirclean:
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

