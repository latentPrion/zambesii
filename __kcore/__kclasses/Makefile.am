noinst_LIBRARIES=lib__kclasses.a
lib__kclasses_a_SOURCES=memBmp.cpp bitmap.cpp allocTable.cpp pageTableCache.cpp slamCache.cpp \
	debugPipe.cpp debugBuffer.cpp memReservoir.cpp heap.cpp cachePool.cpp \
	hardwareIdList.cpp singleWaiterQueue.cpp \
	test_memBmp.cpp

memReservoir.$(OBJEXT): CXXFLAGS+=-Wno-frame-address
test_memBmp.$(OBJEXT): CXXFLAGS+=-Wno-variadic-macros
