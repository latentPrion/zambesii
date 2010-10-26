ifeq ($(ZCXX), i586-elf-g++)
	ZCXXABI = icxxabi
endif

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
	cd __kstdlib/$(ZCXXABI); make clean
	cd __kthreads; make clean
	cd __kthreads/$(ZARCH); make clean
	cd kernel/common; make clean
	cd kernel/$(ZARCH); make clean
	cd kernel/common/memoryTrib; make clean
	cd kernel/common/numaTrib; make clean
	cd kernel/common/cpuTrib; make clean
	cd kernel/common/firmwareTrib; make clean
	cd kernel/common/interruptTrib; make clean
	cd kernel/common/timerTrib; make clean
	cd kernel/common/processTrib; make clean
	cd kernel/common/taskTrib; make clean
	cd commonlibs/libx86mp; make clean
	cd commonlibs/libacpi; make clean
	cd kernel/common/execTrib; make clean

aclean:
	rm -f *.a *.o __klinkScript.ld

fonyphile:
	rm -f clean

