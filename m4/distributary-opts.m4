dnl AX_ZBZ_DTRIB_ARG_ENABLE(feature-name, description, config-define, [unimplemented])
dnl Creates a --enable-dtrib-<feature-name> configure option
dnl
dnl Parameters:
dnl   $1  feature-name     The name of the distributary (e.g., cisternn, levee)
dnl   $2  description      Human-readable description of the distributary
dnl   $3  config-define    The C preprocessor macro to define when enabled
dnl   $4  unimplemented   Optional. If set to "unimplemented", error out when enabled
dnl
dnl The macro will:
dnl   - Create --enable-dtrib-<feature-name> configure option
dnl   - Define <config-define> when enabled
dnl   - Add feature-name to DISTRIBUTARIES_ENABLED_SUBDIRS
dnl   - Error if marked unimplemented and user tries to enable it
AC_DEFUN([AX_ZBZ_DTRIB_ARG_ENABLE], [
    AC_ARG_ENABLE([dtrib-$1],
        [AS_HELP_STRING([--enable-dtrib-$1],
            [Enable $2])],
        [AS_IF([test "x$enable_dtrib_$1" != "xno"], [
            AC_DEFINE_UNQUOTED([$3], ["${enable_dtrib_$1}"], [Enable $2])
            DISTRIBUTARIES_ENABLED_SUBDIRS="${DISTRIBUTARIES_ENABLED_SUBDIRS} $1"
            AS_IF([test "x$4" = "xunimplemented"],
                [AC_MSG_ERROR([$2 is not yet implemented])])
        ])],
        [enable_dtrib_$1=no]
    )
])

AX_ZBZ_DTRIB_ARG_ENABLE([cisternn],
    [Cisternn Distributary (storage)],
    [CONFIG_DTRIB_CISTERNN])
AX_ZBZ_DTRIB_ARG_ENABLE([levee],
    [Levee Distributary (security)],
    [CONFIG_DTRIB_LEVEE],
    [unimplemented])
AX_ZBZ_DTRIB_ARG_ENABLE([aqueductt],
    [Aqueductt Distributary (networking)],
    [CONFIG_DTRIB_AQUEDUCTT],
    [unimplemented])
AX_ZBZ_DTRIB_ARG_ENABLE([reflectionn],
    [Reflectionn Distributary (graphics)],
    [CONFIG_DTRIB_REFLECTIONN],
    [unimplemented])
AX_ZBZ_DTRIB_ARG_ENABLE([caurall],
    [Caurall Distributary (audio)],
    [CONFIG_DTRIB_CAURALL],
    [unimplemented])
AX_ZBZ_DTRIB_ARG_ENABLE([watermarkk],
    [Watermarkk Distributary (user authentication, authorization, and encryption)],
    [CONFIG_DTRIB_WATERMARKK],
    [unimplemented])
AX_ZBZ_DTRIB_ARG_ENABLE([neapp],
    [Neapp Distributary (RAM swapping to storage)],
    [CONFIG_DTRIB_NEAPP],
    [unimplemented])
