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
EXECOBJS=__kthreads.a __karchThreads.a kernel.a archKernel.a \
	platformCore.a memoryTrib.a numaTrib.a firmwareTrib.a \
	cpuTrib.a archCpuTrib.a platformCore.a \
	chipset.a chipsetCore.a archCore.a __kclasses.a \
	__kstdlib.a __kcxxabi.a __kcxxlib.a __kclib.a lang.a
GOAL=../core.o

all: $(GOAL)

# The kernel core file produced here must be relocatable. We have to link it
# with the Endo Kernel FS fragments later on in the kernel build process.
$(GOAL): $(EXECOBJS)
	$(LD) $(LDFLAGS) -T __klinkScript.ld -i -o $(GOAL) \
		-\( $(EXECOBJS) -\)

include clean.mk
include __kstdlib/__kstdlib.mk
include kernel/common/tribs.mk

archCore.a:
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	@echo Building arch core dir.
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	cd arch/$(ZARCH); make

chipset.a:
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	@echo Building chipset/ dir.
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	cd chipset; make

chipsetCore.a:
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	@echo Building chipset core dir.
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	cd chipset/$(ZCHIPSET); make

platformCore.a:
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	@echo Building platform core dir.
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	cd platform/$(ZARCH)-$(ZCHIPSET); make

__kthreads.a:
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	@echo Building __kthreads/ dir.
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	cd __kthreads; make

__karchThreads.a:
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	@echo Building arch __kthreads dir.
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	cd __kthreads/$(ZARCH); make

kernel.a:
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	@echo Building kernel/common/ dir.
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	cd kernel/common; make

archKernel.a:
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	@echo Building arch kernel dir.
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	cd kernel/$(ZARCH); make

__kclasses.a:
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	@echo Building __kclasses/ dir.
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	cd __kclasses; make

lang.a:
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	@echo Building lang/ dir.
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	cd lang/$(ZLANG); make
