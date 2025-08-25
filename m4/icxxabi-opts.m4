# Itanium C++ ABI options configuration

# Maximum number of __cxa_atexit functions configuration
AC_ARG_WITH([cxxabi-atexit-max-nfuncs],
    [AS_HELP_STRING([--with-cxxabi-atexit-max-nfuncs=@<:@integer@:>@],
        [Maximum number of atexit functions supported @<:@default=128@:>@])],
    [], [with_cxxabi_atexit_max_nfuncs="128"]
)
# Validate with_cxxabi_atexit_max_nfuncs is a positive integer
ZBZ_VALIDATE_POSITIVE_NONZERO_INTEGER([with_cxxabi_atexit_max_nfuncs])
AC_DEFINE_UNQUOTED([CONFIG_CXXABI_ATEXIT_MAX_NFUNCS],
    [$with_cxxabi_atexit_max_nfuncs],
    [Maximum number of atexit functions supported])
AC_SUBST([with_cxxabi_atexit_max_nfuncs])
