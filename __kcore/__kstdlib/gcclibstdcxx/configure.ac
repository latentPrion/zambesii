# Process this file with autoreconf to produce a configure script.

AC_INIT([libstdc++-freestanding], [1.0.0], [], [libstdc++-freestanding])
AC_CONFIG_SRCDIR([libstdcxx-v3/src/shared/hashtable-aux.cc])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([zgcclibstdcxx-autoconf-aux])

# Include the AX_SUBDIRS_CONFIGURE macro
m4_include([../../../m4/ax_subdirs_configure.m4])

# Gets build, host, target, *_vendor, *_cpu, *_os, etc.
AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET
target_alias=${target_alias-$host_alias}

# For freestanding environment, we're always cross-compiling
GLIBCXX_IS_NATIVE=false

# In the case that we're building without headers, we won't have <stdio.h>
# available. In these cases, we have to instruct autotools to never include
# <stdio.h> as a part of default headers.
m4_version_prereq([2.70], [], [
	AS_IF([test "x$with_headers" = "xno"], [
	ac_includes_default=`echo "$ac_includes_default" | \
	  sed '/^#include <stdio.h>$/d'`
	])
])

# Sets up automake.  Must come after AC_CANONICAL_SYSTEM.
AM_INIT_AUTOMAKE([1.9.3 no-define foreign no-dependencies no-dist -Wall \
  -Wno-portability -Wno-override])
AH_TEMPLATE(PACKAGE, [Name of package])
AH_TEMPLATE(VERSION, [Version number of package])

# For cross-compilation, we can't create executables
AC_NO_EXECUTABLES

# -fno-builtin must be present here so that a non-conflicting form of
# std::exit can be guessed by AC_PROG_CXX, and used in later tests.
save_CXXFLAGS="$CXXFLAGS"
CXXFLAGS="$CXXFLAGS -fno-builtin"
AC_PROG_CC
AC_PROG_CXX
AC_PROG_RANLIB
CXXFLAGS="$save_CXXFLAGS"

# Check for required utilities
AC_CHECK_PROG([MKDIR_P], [mkdir], [mkdir -p], [no])
AS_IF([test "$MKDIR_P" = "no"], [
    AC_MSG_ERROR([mkdir -p is required but not found])
])

AC_CHECK_PROG([PATCH], [patch], [patch], [no])
AS_IF([test "$PATCH" = "no"], [
    AC_MSG_ERROR([patch is required but not found])
])

AC_SYS_LARGEFILE

# For freestanding environment, disable shared libraries
enable_shared=no
enable_static=yes

# Disable hosted features for freestanding environment
GLIBCXX_HOSTED=false

# Configure EH pool object count
AC_ARG_WITH([eh-pool-obj-count],
    [AS_HELP_STRING([--with-eh-pool-obj-count=N],
                   [number of objects in static EH pool (default: 64)])],
    [eh_pool_obj_count=$withval],
    [eh_pool_obj_count=64])

# Generate DATESTAMP at configure time
AC_CONFIG_COMMANDS_PRE([
    AS_ECHO(["Generating DATESTAMP at configure time..."])
    ${MKDIR_P} "$srcdir/gcc"
    date +%Y%m%d > "$srcdir/gcc/DATESTAMP"
])

# Apply patches to libstdc++-v3 before configuring
AC_CONFIG_COMMANDS_PRE([
    AS_ECHO(["Applying patches to libstdc++-v3 for freestanding environment..."])

    # Apply patch to modify Makefile.am for freestanding
    AS_ECHO(["Patching Makefile.am to disable hosted components..."])
    ${PATCH} -f -d "$srcdir/libstdcxx-v3" -p1 \
        < "$srcdir/patches/02-freestanding-makefile.patch" || true

    # Fix libtool to use parent directory's ltmain.sh
    # This ltmain.sh file in the parent dir is taken from the GCC repo top dir.
    AS_ECHO(["Fixing libtool to use parent directory's ltmain.sh..."])
    ltmain_dir=`cd "$srcdir" && pwd`
    sed -i 's|ltmain="\$ac_aux_dir/ltmain.sh"|ltmain="'"$ltmain_dir"'/ltmain.sh"|g' "$srcdir/libstdcxx-v3/configure"
])

# Configure libstdc++-v3 subdirectory
AX_SUBDIRS_CONFIGURE([libstdcxx-v3],
    [[]],[[]],
    [[--build=${build_alias}],
     [--host=${host_alias}],
     [--target=${target_alias}],
     [--enable-static],
     [--disable-shared],
     [--disable-hosted-libstdcxx],
     [--without-headers],
     [--disable-nls],
     [--disable-multilib],
     [--enable-libstdcxx-static-eh-pool],
     [--with-libstdcxx-eh-pool-obj-count=${eh_pool_obj_count}],
     [--with-target-subdir=freestanding]],
    [[with_target_subdir=freestanding],
     [gcc_no_link=yes],
     [ac_cv_func_fcntl=no],
     [have_fcntl=no],
     [glibcxx_cv_chmod=no],
     [glibcxx_cv_mkdir=no],
     [glibcxx_cv_chdir=no],
     [glibcxx_cv_getcwd=no],
     [glibcxx_cv_realpath=no],
     [glibcxx_cv_utimensat=no],
     [glibcxx_cv_utime=no],
     [glibcxx_cv_lstat=no],
     [glibcxx_cv_st_mtim=no],
     [glibcxx_cv_fchmod=no],
     [glibcxx_cv_fchmodat=no],
     [glibcxx_cv_link=no],
     [glibcxx_cv_lseek=no],
     [glibcxx_cv_readlink=no],
     [glibcxx_cv_symlink=no],
     [glibcxx_cv_truncate=no],
     [glibcxx_cv_copy_file_range=no],
     [glibcxx_cv_sendfile=no],
     [glibcxx_cv_fdopendir=no],
     [glibcxx_cv_dirfd=no],
     [glibcxx_cv_openat=no],
     [glibcxx_cv_unlinkat=no],
     [glibcxx_cv_dirent_d_type=no],
     [ac_cv_header_fcntl_h=no],
     [ac_cv_header_dirent_h=no],
     [ac_cv_header_sys_statvfs_h=no],
     [ac_cv_header_utime_h=no],
     [ac_cv_header_sys_ioctl_h=no],
     [ac_cv_header_sys_socket_h=no],
     [ac_cv_header_sys_uio_h=no],
     [ac_cv_header_poll_h=no],
     [ac_cv_header_netdb_h=no],
     [ac_cv_header_arpa_inet_h=no],
     [ac_cv_header_netinet_in_h=no],
     [ac_cv_header_netinet_tcp_h=no],
     [ac_cv_decl_F_GETFL=no],
     [ac_cv_decl_F_SETFL=no]])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
