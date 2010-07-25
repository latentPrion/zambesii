.PHONY=

#
# Zambezii Kernel top-level makefile.
#

iso: zambezii.iso
	cp -f zambezii.zxe iso/zambezii/core
	genisoimage -R -b boot/grub/stage2_eltorito -no-emul-boot \
	-boot-load-size 4 -boot-info-table -V zambezii -o zambezii.iso iso

zambezii.iso: exec

exec: zambezii.zxe

zambezii.zxe: __kcore drivers libraries resources programs
	$(LD) -T core/__klinkScript.ld -o $@ \
		core.o

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

# Top level clean target
clean: fonyphile
	rm -f *.o *.ekf *.zxe *.iso *.img
	cd core; make clean
#	cd programs; make clean
#	cd libraries; make clean
#	cd resources; make clean
#	cd drivers; make clean

aclean: fonyphile
	rm -f *.o *.ekf *.zxe *.iso *.img
	cd core; make aclean
#	cd programs; make aclean
#	cd libraries; make aclean
#	cd resources; make aclean
#	cd drivers; make aclean

fonyphile:
	rm -f clean aclean

