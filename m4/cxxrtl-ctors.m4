# C++ Runtime Library Constructor Detection
# This file contains macros to detect how the user's supplied cross compiler
# handles global constructors.

# Helper function to extract first word from objdump section output
# Usage: extract_first_word <section_name> <object_file>
extract_first_word() {
    local section_name="$1"
    local object_file="$2"
    ${OBJDUMP} -s -j "$section_name" "$object_file" 2>/dev/null |
        grep "^ [0-9a-f]" | head -1 | awk '{print $2}'
}

# Helper function to test if a hex word is a kernel virtual address space
# address. Uses a heuristic based on the first digit of the hex word. Since the
# kernel is usually linked in the high half of every vaddrspace, we can assume
# that any hex word with a first digit >= 8 is a kernel virtual address space
# address.
#
# Usage: is___kvaddrspace_vaddr <hex_word>
is___kvaddrspace_vaddr() {
    local hex_word="$1"
    test -n "$hex_word" && test "$(printf "%d" "0x${hex_word:0:1}")" -ge 8
}

# Define the test program once
m4_define([ZBZ_CTOR_TEST_PROGRAM], [
class TestConstructor {
public:
    TestConstructor(const char* name)
    : name_(name)
    {}
private:
    const char* name_;
};

TestConstructor ctor1("ctor1");
TestConstructor ctor2("ctor2");
TestConstructor ctor3("ctor3");

/* Minimal main function to ensure object file generation
 * The constructors will be analyzed regardless of main
 */
int main(void) {
    return 0;
}

/* Entry point for freestanding linking. The linker should automatically use
 * this _start symbol as the program entry point.
 */
extern "C" void _start(void) {
    main();
}
])

# Detect constructor packaging method
# Must only be called after AM_*FLAGS have been set.
AC_DEFUN([ZBZ_DETECT_CTOR_PACKAGING], [
    AC_MSG_CHECKING([for C++ global constructor packaging method])

    # Save current flags
    ZBZ_SAVED_CPPFLAGS="$CPPFLAGS"
    ZBZ_SAVED_CFLAGS="$CFLAGS"
    ZBZ_SAVED_CXXFLAGS="$CXXFLAGS"
    ZBZ_SAVED_LDFLAGS="$LDFLAGS"

    # Set flags for freestanding compilation
    CPPFLAGS="$AM_CPPFLAGS"
    CFLAGS="${AM_CFLAGS} -Wno-pedantic"
    CXXFLAGS="${AM_CXXFLAGS} -Wno-pedantic"
    LDFLAGS="$AM_LDFLAGS"

    # Switch to C++ language mode and try to compile object file
    AC_LANG_PUSH([C++])
    AC_COMPILE_IFELSE([AC_LANG_SOURCE(ZBZ_CTOR_TEST_PROGRAM)], [
# Compilation succeeded, copy object file for inspection
if test -f conftest.${OBJEXT}; then
cp conftest.${OBJEXT} packaging-detection-test.o
AC_MSG_NOTICE([Object file saved for inspection: packaging-detection-test.o])
fi
# Analyze the object file
        # Check which sections contain constructor data.
        AS_IF([${OBJDUMP} -h conftest.${OBJEXT} 2>/dev/null | grep -q "\.ctors"], [
            ctor_packaging="ctors_section"
            AC_DEFINE([CONFIG_CXXRTL_CTORS_PACKAGING_CTORS_SECTION], [1],
                [Global constructors packaged in .ctors section])
        ], [${OBJDUMP} -h conftest.${OBJEXT} 2>/dev/null | grep -q "\.init_array"], [
	    # The ordering of our checks is important here. We want to
	    # detect the .init_array section first, because .init 's name string
	    # is a substring of .init_array 's name string.
            ctor_packaging="init_array_section"
            AC_DEFINE([CONFIG_CXXRTL_CTORS_PACKAGING_INIT_ARRAY_SECTION], [1],
                [Global constructors packaged in .init_array section])
        ], [${OBJDUMP} -h conftest.${OBJEXT} 2>/dev/null | grep -q "\.init"], [
            ctor_packaging="init_section"
            AC_DEFINE([CONFIG_CXXRTL_CTORS_PACKAGING_INIT_SECTION], [1],
                [Global constructors packaged in .init section])
        ], [
            ctor_packaging="unknown"
            AC_MSG_FAILURE(m4_normalize([Failed to detect C++ global constructor
		packaging method. Use
		--with-cxxrtl-ctor-packaging to specify manually.]))
        ])
    ], [
        ctor_packaging="unknown"
        AC_MSG_FAILURE(m4_normalize([Failed to compile test program to detect
		C++ global constructor packaging method.
		Use --with-cxxrtl-ctor-packaging to specify manually.]))
    ])

    # Restore language mode
    AC_LANG_POP([C++])
    # Clean up
    rm -f conftest.cpp conftest.${OBJEXT}

    # Restore flags
    CPPFLAGS="$ZBZ_SAVED_CPPFLAGS"
    CFLAGS="$ZBZ_SAVED_CFLAGS"
    CXXFLAGS="$ZBZ_SAVED_CXXFLAGS"
    LDFLAGS="$ZBZ_SAVED_LDFLAGS"

    AC_MSG_RESULT([$ctor_packaging])
])

# Detect constructor call method
# Must only be called after AM_*FLAGS have been set.
AC_DEFUN([ZBZ_DETECT_CTOR_CALL_METHOD], [
    AC_MSG_CHECKING([for C++ global constructor call method])

    # Save current flags
    ZBZ_SAVED_CPPFLAGS="$CPPFLAGS"
    ZBZ_SAVED_CFLAGS="$CFLAGS"
    ZBZ_SAVED_CXXFLAGS="$CXXFLAGS"
    ZBZ_SAVED_LDFLAGS="$LDFLAGS"

    # Set flags for freestanding compilation
    CPPFLAGS="$AM_CPPFLAGS"
    CFLAGS="$AM_CFLAGS"
    CXXFLAGS="$AM_CXXFLAGS"
    LDFLAGS="$AM_LDFLAGS"

    # Switch to C++ language mode and try to compile and link test program
    AC_LANG_PUSH([C++])
    ZBZ_LINK_IFELSE([AC_LANG_SOURCE(ZBZ_CTOR_TEST_PROGRAM)], [
        # Compilation and linking succeeded, analyze the linked output.
	# We subtractively decode the first word of each section to deduce
	# the call method. This deduction is not perfect, but it's a good
	# heuristic.
AC_MSG_NOTICE([Got here: seems to have compiled and linked])

        AS_IF([test "$ctor_packaging" = "ctors_section"], [
            # Check .ctors section format
AC_MSG_NOTICE([Debug: Packaging detected .ctors section, but checking linked executable])
AC_MSG_NOTICE([Debug: .ctors section objdump output:])
${OBJDUMP} -s -j .ctors conftest 2>/dev/null | while read line; do
AC_MSG_NOTICE([  $line])
done
            ctor_first_word=$(extract_first_word ".ctors" "conftest")
AC_MSG_NOTICE([Debug: .ctors first word = "$ctor_first_word"])
            AS_IF([test "$ctor_first_word" = "ffffffff"], [
                # First word is -1: indicates possibly prefixed count with null
		# termination, based on the GCC source code.
                ctor_call_method="possibly_prefixed_count_null_terminated"
                AC_DEFINE([CONFIG_CXXRTL_CTORS_CALLMETHOD_POSSIBLY_PREFIXED_COUNT_NULL_TERMINATED],
		    [1], m4_normalize([C++ global constructors use possibly
		    	prefixed count with null termination]))
            ], [test -n "$ctor_first_word" && test "$ctor_first_word" != "ffffffff" &&
                ! is___kvaddrspace_vaddr "$ctor_first_word"], [
                # First word exists, is not -1, and is not a kernel virtual
		# address space address: indicates possibly prefixed count with
		# null termination.
                ctor_call_method="possibly_prefixed_count_null_terminated"
                AC_DEFINE([CONFIG_CXXRTL_CTORS_CALLMETHOD_POSSIBLY_PREFIXED_COUNT_NULL_TERMINATED],
		    [1], m4_normalize([C++ global constructors use possibly
		    	prefixed count with null termination]))
            ], [
                # First word is a kernel virtual address space address:
		# indicates bounded array of function pointers.
                ctor_call_method="bounded"
                AC_DEFINE([CONFIG_CXXRTL_CTORS_CALLMETHOD_BOUNDED], [1],
                    m4_normalize([C++ global constructors use bounded array
		    	of function pointers]))
            ])
        ], [test "$ctor_packaging" = "init_section"], [
            # Check .init section format
AC_MSG_NOTICE([Debug: .init section objdump output:])
${OBJDUMP} -s -j .init conftest 2>/dev/null | while read line; do
AC_MSG_NOTICE([  $line])
done
            init_first_word=$(extract_first_word ".init" "conftest")
AC_MSG_NOTICE([Debug: .init first word = "$init_first_word"])
            AS_IF([test -n "$init_first_word" && is___kvaddrspace_vaddr "$init_first_word"], [
                # First word is a kernel virtual address space address:
		# suggests a bounded array. It's less clear with the .init
		# section, because we could be looking at actual binary
		# instructions. Hence our assumption about the meaning of the
		# top bit of the first word is much less reliable.
                ctor_call_method="bounded"
                AC_DEFINE([CONFIG_CXXRTL_CTORS_CALLMETHOD_BOUNDED], [1],
                    m4_normalize([C++ global constructors use bounded array]))
            ], [
                # First word is not a kernel virtual address space address:
		# suggests a callable section.
                ctor_call_method="callable_section"
                AC_DEFINE([CONFIG_CXXRTL_CTORS_CALLMETHOD_CALLABLE_SECTION], [1],
                    m4_normalize([C++ global constructors use callable
		    	section]))
            ])
        ], [test "$ctor_packaging" = "init_array_section"], [
	    # .init_array section is always a bounded array of function
	    # pointers.
AC_MSG_NOTICE([Debug: .init_array section objdump output:])
${OBJDUMP} -s -j .init_array conftest 2>/dev/null | while read line; do
AC_MSG_NOTICE([  $line])
done

            ctor_call_method="bounded"
            AC_DEFINE([CONFIG_CXXRTL_CTORS_CALLMETHOD_BOUNDED], [1],
                m4_normalize([C++ global constructors use bounded array]))
        ], [
            ctor_call_method="unknown"
            AC_MSG_FAILURE(m4_normalize([Could not determine C++ global
		constructor call method. Use
		--with-cxxrtl-ctor-callmethod to specify manually.]))
        ])
    ], [
        ctor_call_method="unknown"
        AC_MSG_FAILURE(m4_normalize([Failed to compile and link test program to
		detect C++ global constructor call method. Use
		--with-cxxrtl-ctor-callmethod to specify manually.]))
    ])
    # Restore language mode
    AC_LANG_POP([C++])

    # Clean up
    rm -f conftest.cpp conftest.${OBJEXT} conftest

    # Restore flags
    CPPFLAGS="$ZBZ_SAVED_CPPFLAGS"
    CFLAGS="$ZBZ_SAVED_CFLAGS"
    CXXFLAGS="$ZBZ_SAVED_CXXFLAGS"
    LDFLAGS="$ZBZ_SAVED_LDFLAGS"

    AC_MSG_RESULT([$ctor_call_method])
])

# Main macro to detect constructor handling
AC_DEFUN([ZBZ_DETECT_CXXRTL_CTORS], [
    # Check if we have objdump available
    AC_CHECK_PROG([OBJDUMP], [objdump], [objdump], [no])
    AS_IF([test "$OBJDUMP" = "no"], [
        AC_MSG_FAILURE(m4_normalize([objdump is required for C++ global
		constructor packaging and call method detection]))
    ])

    # Detect C++ global constructor packaging method if not overridden
    AS_IF([test -z "$ctor_packaging"], [
        ZBZ_DETECT_CTOR_PACKAGING
    ])

    # Detect C++ global constructor call method if not overridden
    AS_IF([test -z "$ctor_call_method"], [
        ZBZ_DETECT_CTOR_CALL_METHOD
    ])

    # Set the appropriate defines based on packaging method
    AS_CASE(["$ctor_packaging"],
        [ctors_section],
            [AC_DEFINE([CONFIG_CXXRTL_CTORS_PACKAGING_CTORS_SECTION], [1],
                [Global constructors packaged in .ctors section])],
        [init_section],
            [AC_DEFINE([CONFIG_CXXRTL_CTORS_PACKAGING_INIT_SECTION], [1],
                [Global constructors packaged in .init section])],
        [init_array_section],
            [AC_DEFINE([CONFIG_CXXRTL_CTORS_PACKAGING_INIT_ARRAY_SECTION], [1],
                [Global constructors packaged in .init_array section])]
    )

    # Set the appropriate defines based on call method
    AS_CASE(["$ctor_call_method"],
        [bounded],
            [AC_DEFINE([CONFIG_CXXRTL_CTORS_CALLMETHOD_BOUNDED], [1],
                [C++ global constructors use bounded array])],
        [possibly_prefixed_count_null_terminated],
            [AC_DEFINE([CONFIG_CXXRTL_CTORS_CALLMETHOD_POSSIBLY_PREFIXED_COUNT_NULL_TERMINATED], [1],
                [C++ global constructors use possibly prefixed count with null termination])],
        [null_terminated],
            [AC_DEFINE([CONFIG_CXXRTL_CTORS_CALLMETHOD_NULL_TERMINATED], [1],
                [C++ global constructors use null termination])],
        [callable_section],
            [AC_DEFINE([CONFIG_CXXRTL_CTORS_CALLMETHOD_CALLABLE_SECTION], [1],
                [C++ global constructors use callable section])]
    )

    # Set default values if detection failed
    AS_IF([test "$ctor_packaging" = "unknown"], [
        AC_MSG_FAILURE(m4_normalize([Could not determine C++ global constructor
		packaging method. Use --with-cxxrtl-ctor-packaging to specify
		manually.]))
    ])

    AS_IF([test "$ctor_call_method" = "unknown"], [
        AC_MSG_FAILURE(m4_normalize([Could not determine C++ global constructor
		call method. Use --with-cxxrtl-ctor-callmethod to specify
		manually.]))
    ])

    # Export variables for Makefiles
    AC_SUBST([ctor_packaging])
    AC_SUBST([ctor_call_method])
])
