.PHONY=

# Try not to change the order of these items. This is the order the linker
# runs through them in. If you change the order around, you'll begin to get
# undefined symbol complaints.
#
# As a rule, make sure to put object files with references to external symbols
# before object files without unresolved symbols. LD seems to run through all
# files resolving as it goes, so if it sees an unresolved symbol too late,
# it halts saying that the symbol cannot be resolved, even if the symbol is
# defined in one of the objects passed on the command line.
EXECOBJS=__kthreads.a kernel.a platform.a \
	memoryTrib.a numaTrib.a cpuTrib.a \
	firmware.a chipset.a arch.a __kclasses.a \
	__kcxxabi.a __kstdlib.a __kcxxlib.a __kclib.a lang.a
GOAL=../core.o

all: $(GOAL)

# The kernel core file produced here must be relocatable. We have to link it
# with the Endo Kernel FS fragments later on in the kernel build process.
$(GOAL): $(EXECOBJS)
	$(LD) $(LDFLAGS) -T platform/__klinkScript.ld -i -o $(GOAL) \
		-\( $(EXECOBJS) -\)

include clean.mk
include __kstdlib/__kstdlib.mk
include kernel/common/tribs.mk

chipset.a:
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	@echo Building chipset/ dir.
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	cd chipset; make

__kthreads.a:
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	@echo Building __kthreads/ dir.
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	cd __kthreads; make

kernel.a:
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	@echo Building kernel/common/ dir.
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	cd kernel/common; make

__kstdlib.a:
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	@echo Building __kstdlib/ dir.
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	cd __kstdlib; make

__kclasses.a:
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	@echo Building __kclasses/ dir.
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	cd __kclasses; make

platform.a:
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	@echo Building platform/ dir.
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	cd platform; make

arch.a:
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	@echo Building arch/ dir.
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	cd arch; make

firmware.a:
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	@echo Building firmware/ dir.
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	cd firmware; make

lang.a:
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	@echo Building lang/ dir.
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	cd lang/common; make

