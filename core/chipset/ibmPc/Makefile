.PHONY=
DELIVERABLE=../../chipsetCore.a
OBJFILES=regionMap.o i8259a.o findTables.o irqControl.o \
	memoryDetection.o terminalMod.o zkcmCore.o ibmPc_modules.o \
	cpuDetection.o memoryAreas.o zkcmIbmPcState.o timerControl.o \
	rtcmos.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean: fonyphile
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

