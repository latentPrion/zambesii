AC_DEFUN([AX_ZBZ_DTRIB_ARG_ENABLE], [
    AC_ARG_ENABLE([dtrib-$1],
        [AS_HELP_STRING([--enable-dtrib-$1],
            [Enable $2])],
        [AS_CASE(["$enableval"], [no], [$4=no],
            [yes | "" | *], [
                $4=yes
                AC_DEFINE([$3], [1], [Enable $2])
                DISTRIBUTARIES_ENABLED_SUBDIRS="${DISTRIBUTARIES_ENABLED_SUBDIRS} $1"
                AS_IF([test "x$5" = "xunimplemented"],
                    [AC_MSG_ERROR([$2 is not yet implemented])])])
        ]
    )
])

AX_ZBZ_DTRIB_ARG_ENABLE([cisternn],
    [Cisternn Distributary (storage)],
    [CONFIG_DTRIB_CISTERNN], [enable_dtrib_cisternn])
AX_ZBZ_DTRIB_ARG_ENABLE([levee],
    [Levee Distributary (security)],
    [CONFIG_DTRIB_LEVEE], [enable_dtrib_levee],
    [unimplemented])
AX_ZBZ_DTRIB_ARG_ENABLE([aqueductt],
    [Aqueductt Distributary (networking)],
    [CONFIG_DTRIB_AQUEDUCTT], [enable_dtrib_aqueductt],
    [unimplemented])
AX_ZBZ_DTRIB_ARG_ENABLE([reflectionn],
    [Reflectionn Distributary (graphics)],
    [CONFIG_DTRIB_REFLECTIONN], [enable_dtrib_reflectionn],
    [unimplemented])
AX_ZBZ_DTRIB_ARG_ENABLE([caurall],
    [Caurall Distributary (audio)],
    [CONFIG_DTRIB_CAURALL], [enable_dtrib_caurall],
    [unimplemented])
AX_ZBZ_DTRIB_ARG_ENABLE([watermarkk],
    [Watermarkk Distributary],
    [CONFIG_DTRIB_WATERMARKK], [enable_dtrib_watermarkk],
    [unimplemented])
