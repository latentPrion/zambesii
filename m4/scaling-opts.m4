AC_ARG_ENABLE([scaling],
    [AS_HELP_STRING([--enable-scaling=@<:@up|smp|numa@:>@],
        [Select processor scaling mode @<:@default=numa@:>@])],
    [AS_CASE(["$enableval"],
        [uniprocessor|up|no],
            [ZSCALING="up"],
        [multiprocessor|multiprocessing|smp|yes],
            [ZSCALING="smp"],
        [numa|ccnuma],
            [ZSCALING="numa"],
        [*], [AC_MSG_ERROR([Invalid scaling mode: $enableval])])],
    [
        ZSCALING="numa"
    ]
)

# Set the appropriate define based on ZSCALING
AS_CASE(["$ZSCALING"],
    [up],
        [AC_DEFINE([CONFIG_UNI_PROCESSOR], [1], [Uniprocessor scaling mode])],
    [smp],
        [AC_DEFINE([CONFIG_SMP], [1], [SMP scaling mode])],
    [numa|*],
        [AC_DEFINE([CONFIG_CC_NUMA], [1], [CC-NUMA scaling mode])]
)
AC_SUBST([ZSCALING])

# Maximum number of CPUs configuration
AC_ARG_WITH([max-ncpus],
    [AS_HELP_STRING([--with-max-ncpus=@<:@integer@:>@],
        [Maximum number of CPUs supported @<:@default=256 or 1 for uniprocessor@:>@])],
    [ZMAX_NCPUS="$withval"],
    [ZMAX_NCPUS="256"]
)

# Force ZMAX_NCPUS to 1 for uniprocessor builds
AS_IF([test "$ZSCALING" = "up"], [ZMAX_NCPUS="1"])
# Validate ZMAX_NCPUS is a positive integer
ZBZ_VALIDATE_POSITIVE_NONZERO_INTEGER([ZMAX_NCPUS])
# Define CONFIG_MAX_NCPUS
AC_DEFINE_UNQUOTED([CONFIG_MAX_NCPUS], [$ZMAX_NCPUS], [Maximum number of CPUs supported])
AC_SUBST([ZMAX_NCPUS])
