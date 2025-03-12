# Enable all debugging options by default if --enable-all-debug-opts is set.
AC_ARG_ENABLE([all-debug-opts],
    [AS_HELP_STRING([--enable-all-debug-opts],
    m4_normalize(
        [Enable all debugging options by default.
        You can then selectively disable them using --disable-<option>]))],
    [AS_IF([test "x$enable_all_debug_opts" != "xno"],
        [AC_DEFINE([CONFIG_ALL_DEBUG_OPTS], [1],
            [Enable all debugging options])]
    )],
    [enable_all_debug_opts=no]
)

dnl AX_ZBZ_DEBUG_ARG_ENABLE(feature-name, description, config-define, [unimplemented])
dnl Creates a --enable-debug-<feature-name> configure option
dnl
dnl Parameters:
dnl   $1  feature-name     The name of the debug feature (without debug- prefix)
dnl   $2  description      Human-readable description of the debug feature
dnl   $3  config-define    The C preprocessor macro to define when enabled
dnl   $4  unimplemented   Optional. If set to "unimplemented", error out when enabled
AC_DEFUN([AX_ZBZ_DEBUG_ARG_ENABLE], [
    AC_ARG_ENABLE([debug-$1],
        [AS_HELP_STRING([--enable-debug-$1],
            [Enable $2])],
        [AS_IF([test "x$4" = "xunimplemented"],
            [AC_MSG_ERROR([$2 is not yet implemented])])],
        [AS_IF([test "x$enable_all_debug_opts" = "xyes"],
            [enable_debug_$1=yes], [enable_debug_$1=no])]
    )

    AS_IF([test "x$enable_debug_$1" != "xno"], [
        AC_DEFINE([$3], [1], [Enable $2])
    ])
])

AX_ZBZ_DEBUG_ARG_ENABLE([locks], [Lock debugging],
    [CONFIG_DEBUG_LOCKS])

AX_ZBZ_DEBUG_ARG_ENABLE([scheduler],
    [Enable debugging for the scheduler],
    [CONFIG_DEBUG_SCHEDULER])
