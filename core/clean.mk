ifeq ($(ZCXX), i586-elf-g++)
	ZCXXABI = icxxabi
endif

clean: fonyphile
	rm -f *.a __klinkScript.ld
	rm -f *.o *.s
	cd arch/$(ZARCH); $(MAKE) clean
	cd chipset; $(MAKE) clean
	cd chipset/$(ZCHIPSET); $(MAKE) clean
	cd firmware/$(ZFIRMWARE); $(MAKE) clean
	cd platform/$(ZARCH)-$(ZCHIPSET); $(MAKE) clean
	cd __kclasses; $(MAKE) clean
	cd __kstdlib; $(MAKE) clean
	cd __kstdlib/$(ZCXXABI); $(MAKE) clean
	cd __kthreads; $(MAKE) clean
	cd __kthreads/$(ZARCH); $(MAKE) clean
	cd kernel/common; $(MAKE) clean
	cd kernel/$(ZARCH); $(MAKE) clean
	cd kernel/common/memoryTrib; $(MAKE) clean
	cd kernel/common/cpuTrib; $(MAKE) clean
	cd kernel/common/interruptTrib; $(MAKE) clean
	cd kernel/common/timerTrib; $(MAKE) clean
	cd kernel/common/processTrib; $(MAKE) clean
	cd kernel/common/taskTrib; $(MAKE) clean
	cd commonlibs/libx86mp; $(MAKE) clean
	cd commonlibs/libacpi; $(MAKE) clean
	cd commonlibs/drivers; $(MAKE) clean
	cd commonlibs/metalanguages; $(MAKE) clean
	cd commonlibs/libzbzcore; $(MAKE) clean
	cd kernel/common/execTrib; $(MAKE) clean
	cd kernel/common/vfsTrib; $(MAKE) clean
	cd kernel/common/debugTrib; $(MAKE) clean
	cd kernel/common/distributaryTrib; $(MAKE) clean
	cd kernel/common/floodplainn; $(MAKE) clean

aclean:
	rm -f *.a *.o *.s __klinkScript.ld *.zudi-index

fonyphile:
	rm -f clean

