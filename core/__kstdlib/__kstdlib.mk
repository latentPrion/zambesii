# __kcxxabi will soon be transformed into a compiler specific directory which
# will auto-build the appropriate C++ runtime for the current compiler.

ifeq ($(ZCXX), i586-elf-g++)
	ZCXXABI = icxxabi
endif

__kcxxabi.a:
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	@echo Building __kstdlib/__kcxxabi/ dir.
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	cd __kstdlib/$(ZCXXABI); make

__kstdlib.a:
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	@echo Building __kstdlib/ dir.
	@echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	cd __kstdlib; make

