AC_INIT([Zambesii UDIProps Indexer], [0.01.000],
    [latentprion@gmail.com],
    [zudi], [github.com/latentPrion/zambesii])

AC_CONFIG_SRCDIR([${ac_top_srcdir}/../../programs/zudi/zudiindex/zudiindex.cpp])
AC_CONFIG_AUX_DIR([autotools-aux])

AM_INIT_AUTOMAKE([foreign subdir-objects -Wall -Woverride])

AC_PROG_CC
AC_PROG_CXX
AC_PROG_LN_S
AC_PATH_PROG([CHMOD], [chmod])
AS_IF([test -z "$CHMOD"], [
    AC_MSG_ERROR([Could not find 'chmod' in PATH. Please install it or add it to PATH.])])

AM_CPPFLAGS=m4_normalize(["-I\"\$(abs_top_srcdir)/../../include\" \
    -I\"\$(abs_top_srcdir)/../../programs/zudi/include\""])
AM_CFLAGS=m4_normalize(["-O3 -Wall -Wextra -pedantic -std=c99"])
AM_CXXFLAGS=m4_normalize(["-O3 -Wall -Wextra -pedantic -std=c++03"])

AC_SUBST([AM_CPPFLAGS])
AC_SUBST([AM_CFLAGS])
AC_SUBST([AM_CXXFLAGS])
AC_SUBST([CHMOD])

AC_CONFIG_HEADERS([config.h])

AC_CONFIG_FILES([
    Makefile zudiindex/Makefile
])

AC_OUTPUT
