.PHONY=
DELIVERABLE=../../chipsetCore.a
_OBJFILES=regionMap.o i8259a.o findTables.o irqControl.o \
	memoryDetection.o vgaTerminal.o zkcmCore.o \
	cpuDetection.o memoryAreas.o zkcmIbmPcState.o timerControl.o \
	rtcmos.o isa_busPinMappings.o busPinMappings.o i8254.o \
	udi_init_info.o i8254_pit.o rs232.o chipset_root_devices.o

THISDIR:=$(patsubst %/,%,$(dir $(abspath $(lastword ${MAKEFILE_LIST}))))
OBJFILES:=$(addprefix ${THISDIR}/, ${_OBJFILES})

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean: fonyphile
	rm -rf *.o *.s *.zudi-index

fonyphile:
	rm -f clean aclean dirclean

