
libx86mp.a:
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	@echo Building commonlibs/libx86mp/ dir.
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	cd commonlibs/libx86mp; make

libacpi.a:
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	@echo Building commonlibs/libacpi/ dir.
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	cd commonlibs/libacpi; make

libzudiIndexParser.o: commonlibs/libzudiIndexParser.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ -c $<

