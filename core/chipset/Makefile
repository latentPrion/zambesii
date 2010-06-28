.PHONY=
DELIVERABLE=../chipset.a
OBJFILES=__kmemory.o memoryConfig.o memoryMap.o regionMap.o numaMap.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

memoryConfig.o: memoryConfig.cpp */memoryConfig.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ -c memoryConfig.cpp

memoryMap.o: memoryMap.cpp */memoryMap.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ -c memoryMap.cpp

regionMap.o: regionMap.cpp */regionMap.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ -c regionMap.cpp

numaMap.o: numaMap.cpp */numaMap.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ -c numaMap.cpp

clean: aclean dirclean

aclean:

dirclean:
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

