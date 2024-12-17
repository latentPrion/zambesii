DIST_SUBDIRS=cpuTrib debugTrib distributaryTrib execTrib \
	floodplainn interruptTrib memoryTrib processTrib taskTrib \
	timerTrib vfsTrib
SUBDIRS=cpuTrib debugTrib distributaryTrib execTrib \
	floodplainn interruptTrib memoryTrib processTrib taskTrib \
	timerTrib vfsTrib

noinst_LIBRARIES=libkernel.a
libkernel_a_SOURCES=__kheaders.cpp __koptimizationHacks.cpp __kclassInstances.cpp \
	multipleReaderLock.cpp waitLock.cpp recursiveLock.cpp vSwamp.cpp panic.cpp \
	numaMemoryBank.cpp numaMemoryRange.cpp thread.cpp process.cpp numaCpuBank.cpp \
	messageStream.cpp zasyncStream.cpp
