clean: fonyphile
	rm -f *.a __klinkScript.ld
	rm -f *.o *.s
	cd arch/$(ZARCH); make clean
	cd chipset; make clean
	cd chipset/$(ZCHIPSET); make clean
	cd firmware/$(ZFIRMWARE); make clean
	cd platform/$(ZARCH)-$(ZCHIPSET); make clean
	cd __kclasses; make clean
	cd __kstdlib; make clean
	cd __kstdlib/__kcxxabi; make clean
	cd __kstdlib/__kcxxlib; make clean
	cd __kstdlib/__kclib; make clean
	cd __kthreads; make clean
	cd __kthreads/$(ZARCH); make clean
	cd kernel/common; make clean
	cd kernel/$(ZARCH); make clean
	cd kernel/common/memoryTrib; make clean
	cd kernel/common/numaTrib; make clean
	cd kernel/common/cpuTrib; make clean
	cd kernel/$(ZARCH)/cpuTrib; make clean
	cd kernel/common/firmwareTrib; make clean

aclean:
	rm -f *.a *.o __klinkScript.ld

fonyphile:
	rm -f clean

