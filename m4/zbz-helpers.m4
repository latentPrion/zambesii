# Define a helper macro to validate positive integers
m4_define([ZBZ_VALIDATE_POSITIVE_NONZERO_INTEGER], [
    AS_IF([! test "$$1" -eq "$$1" 2>/dev/null || test "$$1" -le 0 2>/dev/null], [
        AC_MSG_ERROR([$1 must be a positive non-zero integer. Value was: $$1])
    ])
])

m4_define([ZBZ_VALIDATE_POSITIVE_INTEGER], [
    AS_IF([! test "$$1" -eq "$$1" 2>/dev/null || test "$$1" -lt 0 2>/dev/null], [
        AC_MSG_ERROR([$1 must be a positive integer. Value was: $$1])
    ])
])

# Helper macro to split version string into components
# Takes PACKAGE_VERSION (e.g. "1.2.3") and creates:
# - ZVERSION_MAJOR (1)
# - ZVERSION_MINOR (2)
# - ZVERSION_PATCHLEVEL (3)
AC_DEFUN([ZBZ_SPLIT_VERSION], [
    # Extract version components using sed
    ZVERSION_MAJOR=`echo $PACKAGE_VERSION | sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    ZVERSION_MINOR=`echo $PACKAGE_VERSION | sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    ZVERSION_PATCHLEVEL=`echo $PACKAGE_VERSION | sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`

    # Validate that we have numbers
    AS_IF([test -z "$ZVERSION_MAJOR" -o -z "$ZVERSION_MINOR" -o -z "$ZVERSION_PATCHLEVEL"],
        [AC_MSG_ERROR([Invalid version format: $PACKAGE_VERSION (expected x.y.z)])])

    # Validate that all components are positive integers
    ZBZ_VALIDATE_POSITIVE_INTEGER([ZVERSION_MAJOR])
    ZBZ_VALIDATE_POSITIVE_INTEGER([ZVERSION_MINOR])
    ZBZ_VALIDATE_POSITIVE_INTEGER([ZVERSION_PATCHLEVEL])

    # Define the version components for config.h
    AC_DEFINE_UNQUOTED([ZVERSION_MAJOR], [$ZVERSION_MAJOR], [Major version number])
    AC_DEFINE_UNQUOTED([ZVERSION_MINOR], [$ZVERSION_MINOR], [Minor version number])
    AC_DEFINE_UNQUOTED([ZVERSION_PATCHLEVEL], [$ZVERSION_PATCHLEVEL], [Patch level])

    # Make them available to Makefiles
    AC_SUBST([ZVERSION_MAJOR])
    AC_SUBST([ZVERSION_MINOR])
    AC_SUBST([ZVERSION_PATCHLEVEL])
])

# Generic macro to compile and then link a test program
# Usage: ZBZ_LINK_IFELSE([source], [action-if-success], [action-if-failure])
# This macro compiles the source using AC_COMPILE_IFELSE, then links the
# resulting object file using the appropriate compiler frontend with LDFLAGS.
AC_DEFUN([ZBZ_LINK_IFELSE], [
    # First compile the test program using AC_COMPILE_IFELSE
    AC_COMPILE_IFELSE([$1], [
        # Compilation succeeded, now link the object file
        # Set appropriate compiler frontend and flags based on current language mode
AC_MSG_NOTICE([Got here: seems to have compiled])
        AS_IF([test "$ac_ext" = "cc" -o "$ac_ext" = "cpp" -o "$ac_ext" = "cxx"], [
            zbz_linker="$CXX"
            zbz_flags="$CXXFLAGS"
        ], [
            zbz_linker="$CC"
            zbz_flags="$CFLAGS"
        ])

        # Link using the selected frontend with appropriate flags
AC_MSG_NOTICE([Linking command: $zbz_linker $zbz_flags $LDFLAGS -o conftest conftest.${OBJEXT}])
        AS_IF([$zbz_linker $zbz_flags $LDFLAGS -o conftest conftest.${OBJEXT} 2>&1], [
# Copy the conftest file immediately after creation
AC_MSG_NOTICE([Attempting to copy conftest file...])
if test -f conftest; then
cp conftest ctor-detection-test.elf
AC_MSG_NOTICE([Conftest file copied successfully: ctor-detection-test.elf])
else
AC_MSG_NOTICE([Conftest file not found for copying])
fi
            $2
        ], [
            $3
        ])
    ], [
        # Compilation failed, execute failure action
        $3
    ])
])
