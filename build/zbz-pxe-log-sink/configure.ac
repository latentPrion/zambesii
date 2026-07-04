AC_INIT([Zambesii PXE log sink], [0.01.000],
    [latentprion@gmail.com],
    [zbz-pxe-log-sink], [github.com/latentPrion/zambesii])

AC_CONFIG_SRCDIR([${ac_top_srcdir}/src/main.cpp])
AC_CONFIG_AUX_DIR([autotools-aux])

AM_INIT_AUTOMAKE([foreign subdir-objects -Wall -Woverride])

AC_PROG_CXX

AM_CXXFLAGS=m4_normalize(["-O2 -Wall -Wextra -pedantic -std=c++23"])

AC_SUBST([AM_CXXFLAGS])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
