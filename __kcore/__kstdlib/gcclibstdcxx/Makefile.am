## Makefile for freestanding libstdc++ components
##
## This file is part of the Zambesii kernel project.

SUBDIRS = libstdcxx-v3

# We'll build a convenience library that includes only the necessary
# components for exception handling and RTTI
noinst_LIBRARIES = libstdcxx-freestanding.a

# The actual sources will be built by the libstdcxx-v3 subdirectory
# We'll link against the libsupc++ convenience library
libstdcxx_freestanding_a_SOURCES =

# Link against the libsupc++ convenience library from the subdirectory
libstdcxx_freestanding_a_LIBADD = libstdcxx-v3/libsupc++/libsupc++convenience.la

# Include directories
AM_CPPFLAGS = -I$(top_srcdir)/libstdcxx-v3/include \
              -I$(top_srcdir)/libstdcxx-v3/libsupc++ \
              -I$(top_builddir)/libstdcxx-v3/include

# Compiler flags for freestanding environment
AM_CXXFLAGS = -fno-builtin -fno-exceptions -fno-rtti -nostdinc -nostdlib \
              -ffreestanding -fno-stack-protector -fno-threadsafe-statics

ACLOCAL_AMFLAGS = -I . -I .. -I libstdcxx-v3/config
