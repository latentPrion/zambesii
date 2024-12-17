# Enable all debugging options by default if --enable-all-debug-opts is set.
AC_ARG_ENABLE([all-debug-opts],
    [AS_HELP_STRING([--enable-all-debug-opts],
    [Enable all debugging options by default. You can then selectively disable them using --disable-<option>])],
    [AS_CASE(["$enableval"], [no],,
        [yes | "" | *],
            enable_all_debug_opts=yes
            AC_DEFINE([CONFIG_ALL_DEBUG_OPTS], [1], [Enable all debugging options])
    )]
)
AC_ARG_ENABLE([debug-locks],
    [AS_HELP_STRING([--enable-debug-locks], [Enable lock debugging])],
    [enable_debug_locks=$enableval],
    [AS_IF([test "x$enable_all_debug_opts" = "xyes"],
        [enable_debug_locks=yes], [enable_debug_locks=no])]
)

AS_CASE(["$enable_debug_locks"], [no],,
    [yes | "" | *], AC_DEFINE([CONFIG_DEBUG_LOCKS], [1], [Enable lock debugging])
)
