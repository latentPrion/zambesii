AC_ARG_ENABLE([dtrib-cisternn],
    [AS_HELP_STRING([--enable-dtrib-cisternn], [Enable Cisternn Distributary (storage)])],
    [AS_CASE(["$enableval"], [no], enable_dtrib_cisternn=no,
        [yes | "" | *],
            enable_dtrib_cisternn=yes
            AC_DEFINE([CONFIG_DTRIB_CISTERNN], [1], [Enable Cisternn Distributary (storage)])
            DISTRIBUTARIES_ENABLED_SUBDIRS="${DISTRIBUTARIES_ENABLED_SUBDIRS} cisternn"
    )]
)
AC_ARG_ENABLE([dtrib-levee],
    [AS_HELP_STRING([--enable-dtrib-levee], [Enable Levee Distributary (security)])],
    [AS_CASE(["$enableval"], [no], enable_dtrib_levee=no,
        [yes | "" | *],
            enable_dtrib_levee=yes
            AC_DEFINE([CONFIG_DTRIB_LEVEE], [1], [Enable Levee Distributary (security)])
            DISTRIBUTARIES_ENABLED_SUBDIRS="${DISTRIBUTARIES_ENABLED_SUBDIRS} levee"
            AC_MSG_ERROR([Levee is not yet implemented])
    )]
)
AC_ARG_ENABLE([dtrib-aqueductt],
    [AS_HELP_STRING([--enable-dtrib-aqueductt], [Enable Aqueductt Distributary (networking)])],
    [AS_CASE(["$enableval"], [no], enable_dtrib_aqueductt=no,
        [yes | "" | *],
            enable_dtrib_aqueductt=yes
            AC_DEFINE([CONFIG_DTRIB_AQUEDUCTT], [1], [Enable Aqueductt Distributary (networking)])
            DISTRIBUTARIES_ENABLED_SUBDIRS="${DISTRIBUTARIES_ENABLED_SUBDIRS} aqueductt"
            AC_MSG_ERROR([Aqueductt is not yet implemented])
    )]
)
AC_ARG_ENABLE([dtrib-reflectionn],
    [AS_HELP_STRING([--enable-dtrib-reflectionn], [Enable Reflectionn Distributary (graphics)])],
    [AS_CASE(["$enableval"], [no], enable_dtrib_reflectionn=no,
        [yes | "" | *],
            enable_dtrib_reflectionn=yes
            AC_DEFINE([CONFIG_DTRIB_REFLECTIONN], [1], [Enable Reflectionn Distributary (graphics)])
            DISTRIBUTARIES_ENABLED_SUBDIRS="${DISTRIBUTARIES_ENABLED_SUBDIRS} reflectionn"
            AC_MSG_ERROR([Reflectionn is not yet implemented])
    )]
)
AC_ARG_ENABLE([dtrib-caurall],
    [AS_HELP_STRING([--enable-dtrib-caurall], [Enable Caurall Distributary (audio)])],
    [AS_CASE(["$enableval"], [no], enable_dtrib_caurall=no,
        [yes | "" | *],
            enable_dtrib_caurall=yes
            AC_DEFINE([CONFIG_DTRIB_CAURALL], [1], [Enable Caurall Distributary (audio)])
            DISTRIBUTARIES_ENABLED_SUBDIRS="${DISTRIBUTARIES_ENABLED_SUBDIRS} caurall"
            AC_MSG_ERROR([Caurall is not yet implemented])
    )]
)
AC_ARG_ENABLE([dtrib-watermarkk],
    [AS_HELP_STRING([--enable-dtrib-watermarkk], [Enable Watermarkk Distributary])],
    [AS_CASE(["$enableval"], [no], enable_dtrib_watermarkk=no,
        [yes | "" | *],
            enable_dtrib_watermarkk=yes
            AC_DEFINE([CONFIG_DTRIB_WATERMARKK], [1], [Enable Watermarkk Distributary])
            DISTRIBUTARIES_ENABLED_SUBDIRS="${DISTRIBUTARIES_ENABLED_SUBDIRS} watermarkk"
            AC_MSG_ERROR([Watermarkk is not yet implemented])
    )]
)
