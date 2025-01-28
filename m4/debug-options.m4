# Enable all debugging options by default if --enable-all-debug-opts is set.
AC_ARG_ENABLE([all-debug-opts],
    [AS_HELP_STRING([--enable-all-debug-opts],
    m4_normalize(
        [Enable all debugging options by default.
        You can then selectively disable them using --disable-<option>]))],
    [AS_CASE(["$enableval"], [no],,
        [yes | "" | *],
            enable_all_debug_opts=yes
            AC_DEFINE([CONFIG_ALL_DEBUG_OPTS], [1],
                [Enable all debugging options])
    )]
)

AC_DEFUN([AX_ZBZ_DEBUG_ARG_ENABLE], [
    AC_ARG_ENABLE([$1],
        [AS_HELP_STRING([--enable-$1], [$3])],
        [$2=$enableval],
        [AS_IF([test "x$enable_all_debug_opts" = "xyes"],
            [$2=yes], [$2=no])]
    )
    AS_CASE(["$$2"], [yes | "" | *],
        [AC_DEFINE([$4], [1], [$3])])
])

AX_ZBZ_DEBUG_ARG_ENABLE([debug-locks], [enable_debug_locks],
    [Enable lock debugging], [CONFIG_DEBUG_LOCKS])
