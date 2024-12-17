noinst_LIBRARIES=libzbzcore.a
libzbzcore_a_SOURCES=main.cpp distributaryPath.cpp driverPath.cpp devicePath.cpp layout.cpp \
	libzudi.cpp libzudi_buf.cpp libzudi_dma.cpp

devicePath.$(OBJEXT) libzudi.$(OBJEXT) \
	libzudi_buf.$(OBJEXT) libzudi_dma.$(OBJEXT): CXXFLAGS+=-Wno-variadic-macros
