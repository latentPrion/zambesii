DIST_SUBDIRS=cpuTrib debugTrib distributaryTrib execTrib \
	floodplainn interruptTrib memoryTrib processTrib taskTrib \
	timerTrib vfsTrib \
	cisternn # aqueductt levee reflectionn caurall watermarkk
SUBDIRS=cpuTrib debugTrib distributaryTrib execTrib \
	floodplainn interruptTrib memoryTrib processTrib taskTrib \
	timerTrib vfsTrib @DISTRIBUTARIES_ENABLED_SUBDIRS@

noinst_LIBRARIES=libkernel.a
libkernel_a_SOURCES=__kheaders.cpp __koptimizationHacks.cpp __kclassInstances.cpp \
	multipleReaderLock.cpp waitLock.cpp recursiveLock.cpp vSwamp.cpp panic.cpp \
	numaMemoryBank.cpp numaMemoryRange.cpp thread.cpp process.cpp numaCpuBank.cpp \
	messageStream.cpp zasyncStream.cpp
