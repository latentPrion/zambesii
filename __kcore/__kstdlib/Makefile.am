SUBDIRS=__klibc __klibcxx icxxabi

noinst_LIBRARIES=lib__kstdlib.a
lib__kstdlib_a_SOURCES=__kmath.cpp test___kbitManipulation.cpp debug.cpp

test___kbitManipulation.$(OBJEXT): AM_CXXFLAGS+=-Wno-variadic-macros
