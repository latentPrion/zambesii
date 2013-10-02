.PHONY=
DELIVERABLE=../../chipsetCore.a
OBJFILES=regionMap.o i8259a.o findTables.o irqControl.o \
	memoryDetection.o vgaTerminal.o zkcmCore.o \
	cpuDetection.o memoryAreas.o zkcmIbmPcState.o timerControl.o \
	rtcmos.o isa_busPinMappings.o busPinMappings.o i8254.o \
	chipset_udi_index.o

all: Makefile $(DELIVERABLE)

chipset_udi_index.o: udi_index chipset_udi_indexTmp.s
chipset_udi_indexTmp.s: chipset_udi_index.S
	$(CPP) $(CPPFLAGS) $< >$@
udi_index:
	@echo "Creating driver index."
	zudiindex -c le -i .
	@echo "Adding drivers to index."
	zudiindex-addall -drivers . .
	@echo "Adding metalanguages to index."
	zudiindex-addall -metas . .

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean: fonyphile
	rm -rf *.o *.s *.zudi-index

fonyphile:
	rm -f clean aclean dirclean

