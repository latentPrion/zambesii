clean: dirclean aclean

aclean: fonyphile
	rm -f *.a $(GOAL)
	cd arch; make aclean
	cd chipset; make aclean
	cd firmware; make aclean
	cd platform; make aclean
	cd __kclasses; make aclean
	cd __kstdlib; make aclean
	cd __kstdlib/__kcxxabi; make aclean
	cd __kstdlib/__kcxxlib; make aclean
	cd __kstdlib/__kclib; make aclean
	cd __kthreads; make aclean
	cd kernel/common; make aclean
	cd kernel/common/memoryTrib; make aclean
	cd kernel/common/numaTrib; make aclean
	cd kernel/common/cpuTrib; make aclean

dirclean: fonyphile
	rm -f *.o *.s
	cd arch; make dirclean
	cd chipset; make dirclean
	cd firmware; make dirclean
	cd platform; make dirclean
	cd __kclasses; make dirclean
	cd __kstdlib; make dirclean
	cd __kstdlib/__kcxxabi; make dirclean
	cd __kstdlib/__kcxxlib; make dirclean
	cd __kstdlib/__kclib; make dirclean
	cd __kthreads; make dirclean
	cd kernel/common; make dirclean
	cd kernel/common/memoryTrib; make dirclean
	cd kernel/common/numaTrib; make dirclean
	cd kernel/common/cpuTrib; make dirclean

fonyphile:
	rm -f clean aclean dirclean

