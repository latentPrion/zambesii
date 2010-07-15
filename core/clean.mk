clean: fonyphile
	rm -f *.a $(GOAL) __klinkScript.ld
	rm -f *.o *.s
	cd arch; make clean
	cd chipset; make clean
	cd firmware; make clean
	cd platform; make clean
	cd __kclasses; make clean
	cd __kstdlib; make clean
	cd __kstdlib/__kcxxabi; make clean
	cd __kstdlib/__kcxxlib; make clean
	cd __kstdlib/__kclib; make clean
	cd __kthreads; make clean
	cd kernel/common; make clean
	cd kernel/common/memoryTrib; make clean
	cd kernel/common/numaTrib; make clean
	cd kernel/common/cpuTrib; make clean

aclean:
	rm -f *.a *.o

fonyphile:
	rm -f clean

