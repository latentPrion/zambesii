# Itanium C++ ABI options configuration

# Maximum number of __cxa_atexit functions configuration
AC_ARG_WITH([icxxabi-atexit-max-nfuncs],
    [AS_HELP_STRING([--with-icxxabi-atexit-max-nfuncs=@<:@integer@:>@],
        [Maximum number of atexit functions supported @<:@default=128@:>@])],
    [], [with_icxxabi_atexit_max_nfuncs="128"]
)
# Validate with_icxxabi_atexit_max_nfuncs is a positive integer
ZBZ_VALIDATE_POSITIVE_NONZERO_INTEGER([with_icxxabi_atexit_max_nfuncs])
AC_DEFINE_UNQUOTED([CONFIG_ICXXABI_ATEXIT_MAX_NFUNCS],
    [$with_icxxabi_atexit_max_nfuncs],
    [Maximum number of atexit functions supported])
AC_SUBST([with_icxxabi_atexit_max_nfuncs])
