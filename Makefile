.PHONY=

#
# Zambezii Kernel top-level makefile.
#

# We leave it up to the user to compile ekfsutil on his own.
iso: zambezii.iso
	cp -f zambezii.zxe iso/core
	genisoimage -R -b boot/grub/stage2_eltorito -no-emul-boot \
	-boot-load-size 4 -boot-info-table -V zbz-0-00-001 -o zambezii.iso iso

zambezii.iso: exec

exec: zambezii.zxe

zambezii.zxe: __kcore drivers libraries resources programs
	$(LD) -T core/platform/__klinkScript.ld -o $@ \
		core.o

# Top level clean target
clean: aclean dirclean

__kcore: core.o
programs: programs.ekf
libraries: libraries.ekf
resources: resources.ekf
drivers: drivers.ekf

# All of these files are presented by their relevant driectories.
core.o:
	@echo xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
	@echo Building core/
	@echo xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
	cd core; make

programs.ekf:
#	@echo xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
#	@echo Building programs/
#	@echo xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
#	cd programs; make

libraries.ekf:
#	@echo xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
#	@echo Building libraries/
#	@echo xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
#	cd libraries; make

resources.ekf:
#	@echo xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
#	@echo Building resources/
#	@echo xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
#	cd resources; make

drivers.ekf:
#	@echo xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
#	@echo Building drivers/
#	@echo xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
#	cd drivers; make

aclean: fonyphile
	rm -f *.o *.ekf *.zxe *.iso
	cd core; make aclean
#	cd programs; make aclean
#	cd libraries; make aclean
#	cd resources; make aclean
#	cd drivers; make aclean

dirclean: fonyphile
	cd core; make dirclean
#	cd programs; make dirclean
#	cd libraries; make dirclean
#	cd resources; make dirclean
#	cd drivers; make dirclean

fonyphile:
	rm -f clean aclean dirclean

