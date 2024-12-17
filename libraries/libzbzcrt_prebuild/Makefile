#
# This Makefile is used to build the prebuilt version of the ZBZ C runtime library.
# It doesn't support out-of-tree building.
# Just type Make right in here.
#
$(warning !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!)
$(warning !!!Make sure to set ZARCH, and to set HOST to your host toolchain prefix!!!)
$(warning !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!)
ifndef ZARCH
$(error ZARCH is not set. Please set ZARCH to the target architecture. Check the top level configure.ac for architecture name options.)
endif

ORIG_CC := $(CC)

ifdef HOST
	ifeq ($(ORIG_CC),cc)
		CC := $(HOST)-gcc
	else
		CC := $(HOST)-$(if $(ORIG_CC),$(ORIG_CC),gcc)
	endif
	CCAS := $(HOST)-as

else
	CC := $(if $(ORIG_CC),$(ORIG_CC),gcc)
	CCAS := as
endif

AS := $(CCAS)
RANLIB ?= ranlib

CPPFLAGS = -DLIBZBZ_PREBUILD

LIBZBZ_CRT_DIR := ../libzbzcrt/$(ZARCH)

libc.a: crt1.o crt0.o crti.o crtn.o
	$(AR) $(ARFLAGS) $@ $^
	$(RANLIB) $@

crt%.S: $(LIBZBZ_CRT_DIR)/crt%.S
	ln -sf $< $@

crt1.S: $(LIBZBZ_CRT_DIR)/crt0.S
	ln -sf $< $@

clean:
	rm -f *.o *.a *.S
