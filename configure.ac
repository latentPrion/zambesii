AC_INIT([Zambesii Kernel], [0.00.014],
    [latentprion@gmail.com],
    [zambesii],
    [https://github.com/latentPrion/zambesii])

# Stop AC_PROG_[CC|CXX] from adding -g and -O2 to C(XX)FLAGS
# if the user hasn't set them.
true ${CFLAGS:=""} ${CXXFLAGS:=""}

# Some libraries can be lifted out of the Zambesii source tree
# and built in a hosted environment. They use this macro to
# determine whether they are being built in a hosted environment.
AC_DEFINE([__ZAMBESII_KERNEL_SOURCE__], [1],
    [Indicates that files are being built as part of Zambesii source])

AC_CONFIG_SRCDIR([__kcore/kernel/common/__kclassInstances.cpp])
AC_CONFIG_AUX_DIR([autotools-aux])
AC_CONFIG_MACRO_DIRS([m4])

m4_include([m4/zbz-helpers.m4])
# Parse the version string into components.
ZBZ_SPLIT_VERSION

# AC_NO_EXECUTABLES is needed because a freestanding cross compiler
# (such as those required to build Zambesii) shouldn't have a C
# runtime library. This disables autoconf's executable testing since
# our cross compiler can't create executables without runtime support.
AC_NO_EXECUTABLES

# Initialize Automake
AM_INIT_AUTOMAKE([foreign subdir-objects -Wall -Woverride])

# Detect build and host systems
AC_CANONICAL_BUILD
AC_CANONICAL_HOST

# Check for & determine which ISO program to use
AC_PATH_PROGS([ISOPROG], [mkisofs genisoimage])
AS_IF([test -z "$ISOPROG"],
    [AC_MSG_ERROR([Could not find mkisofs or genisoimage in PATH. Please install one of them.])])

# ZBZ flags: for building the Zambesii kernel binaries that will run on the host
ZBZ_CPP_FLAGS=m4_normalize(["-I\"\$(top_srcdir)/include\" \
    -I\"\$(top_srcdir)/programs/zudi/include\" \
    -I\"\$(top_srcdir)/__kcore/include\" \
    -ffreestanding"])
# TODO: If we add suppor for CLang, we should add -fno-use-init-array.
ZBZ_C_CXX_COMMON_FLAGS=m4_normalize(["-O3 -Wstrict-overflow=4 -Wall -Wextra \
    -pedantic -fno-omit-frame-pointer \
    -fno-pic -static -static-libgcc -static-libstdc++"])
ZBZ_C_FLAGS=m4_normalize(["-std=c99"])
ZBZ_CXX_FLAGS=m4_normalize(["-std=c++98 -fno-exceptions -fno-rtti"])
# For some reason, i686-linux-gnu-ld tries to build a position-
# independent binary image by default. Thus we pass -no-pie.
#
#	FIXME:
# Also, it appears as if some executable code may be generated
# on the stack when cross targeting i686-linux-gnu from an
# aarch64-linux-gnu buildmach? Unsure. Investigate.
#
#	EXPLANATION:
# We use -nostartfiles in order to constrain the method that GCC
# uses to assemble the global constructors. Without -nostartfiles,
# GCC can choose any of the methods, from Collect2, to .ctors,
# to .init, to .init_array. The .init_array option is ghoulishly
# undesirable for a kernel; and -nostartfiles basically seems to
# exclude that option; and moreover it seems to force GCC to use
# the Itanium ABI method. The other methods are all able to be
# handled gracefully, even if it doesn't force Itanium ABI
# conformance on all xcompilers.
#
# --no-dynamic-linker tells LD not to emit .interp, or to emit any
# dynamic linking info for a position independent build. We add it
# out of caution.
ZBZ_LD_FLAGS=m4_normalize(["-no-pie -Wl,--no-dynamic-linker -z noexecstack
	-nostartfiles -nodefaultlibs"])

# Add arguments for host machine
AC_ARG_VAR([ZARCH], [Zambesii kernel architecture (mandatory, options: i586, x86_64)])
AC_ARG_VAR([ZSOC], [Zambesii kernel SoC embedding (optional)])
AC_ARG_VAR([ZCHIPSET], [Zambesii kernel chipset (mandatory, options: ibmPc, generic)])
AC_ARG_VAR([ZFIRMWARE], [Zambesii kernel firmware (mandatory, options: ibmPcBios, uefi, generic)])
AC_ARG_VAR([MAKE_IMM_PATH], [Target path for the `make imm` command (optional)])
AS_IF([test -z "${MAKE_IMM_PATH}"], [MAKE_IMM_PATH="/boot"])

# Validate mandatory arguments
AS_IF([test -z "$host_alias"], [AC_MSG_ERROR([--host is mandatory.])])

# Initialize ZCHIPSET_USER_MUST_SET_FIRMWARE to 'no'
ZCHIPSET_USER_MUST_SET_FIRMWARE='no'
ZCHIPSET_COMMONLIBS_ENABLED_SUBDIRS=""
AS_CASE(["$ZCHIPSET"],
    [ibmPc|pc|pcCompatible],
        ZCHIPSET="ibmPc"
        ZCHIPSET_USER_MUST_SET_FIRMWARE='yes'
        AC_DEFINE([CONFIG_CHIPSET_IBM_PC], [1], [IBM PC chipset])
        ZCHIPSET_COMMONLIBS_ENABLED_SUBDIRS="${ZCHIPSET_COMMONLIBS_ENABLED_SUBDIRS} libx86mp libacpi",
    [generic],
        ZFIRMWARE="generic"
        AC_DEFINE([CONFIG_CHIPSET_GENERIC], [1], [Generic chipset]),
    [*],
        AC_MSG_ERROR(m4_normalize(
	        [Unsupported or empty host chipset value: "${ZCHIPSET}".
	        Try setting ZCHIPSET=<CHIPSET>.]))
)
ZCHIPSET_COMMONLIBS_KERNEL_LIBS=""
AS_IF([test -n "${ZCHIPSET_COMMONLIBS_ENABLED_SUBDIRS}"], [
    for lib in ${ZCHIPSET_COMMONLIBS_ENABLED_SUBDIRS}; do
        ZCHIPSET_COMMONLIBS_KERNEL_LIBS="${ZCHIPSET_COMMONLIBS_KERNEL_LIBS} __kcore/commonlibs/${lib}/${lib}.a"
    done
])

# Infer ZARCH from --host triplet if possible
AS_IF([test -z "$ZARCH" && test -n "$host_alias"], [
    host_cpu=`echo ${host_alias} | cut -d'-' -f1`
    AS_CASE(["$host_cpu"],
        [i?86|x86|x86_32], ZARCH="i586",
        [x86_64|x64|amd64|?ntel64], ZARCH="x86_64",
        [*], AC_MSG_WARN([Could not infer ZARCH from --host CPU value: "${host_cpu}"])
    )]
    AC_MSG_NOTICE([Inferred ZARCH (="${ZARCH}") from --host CPU value: (="${host_cpu}")])
)

# Validate ZARCH: allow the user to specify it if --host guess isn't working.
AS_CASE(["$ZARCH"],
    [i?86|x86|x86_32],
        ZARCH="i586"
        AC_DEFINE([CONFIG_ARCH_x86_32], [1], [x86 32-bit architecture])
	# GCC automatically enables -fcf-protection when we enable -march=i586.
	# So we disable it.
	ZBZ_C_CXX_COMMON_FLAGS=m4_normalize(["${ZBZ_C_CXX_COMMON_FLAGS} \
		-march=i586 -fcf-protection=none"]),
    [x86_64|x64|amd64|?ntel64],
        ZARCH="x86_64"
        AC_DEFINE([CONFIG_ARCH_x86_64], [1], [x86 64-bit architecture])
        AC_MSG_ERROR([x86_64 architecture is not yet supported.]),
    [*], AC_MSG_ERROR(m4_normalize(
	[Unsupported or empty host architecture value: "${ZARCH}".
	Try setting --host=<TRIPLET>, or ZARCH=<ARCH>.]))
)

# Validate ZFIRMWARE
AS_CASE(["$ZFIRMWARE"],
    [ibmPcBios|pcBios|bios],
        ZFIRMWARE="ibmPcBios"
        AC_DEFINE([CONFIG_FIRMWARE_IBM_PC_BIOS], [1], [IBM PC BIOS firmware]),
    [uefi|efi],
        ZFIRMWARE="uefi"
        AC_DEFINE([CONFIG_FIRMWARE_UEFI], [1], [UEFI firmware])
        AC_MSG_ERROR([UEFI firmware is not yet supported.]),
    [generic], AC_DEFINE([CONFIG_FIRMWARE_GENERIC], [1], [Generic firmware]),
    [*],
        AS_IF([test "$ZCHIPSET_USER_MUST_SET_FIRMWARE" = 'yes'], [
            AC_MSG_ERROR(m4_normalize(
	            [Unsupported or empty host firmware value: "${ZFIRMWARE}".
	            Try setting ZFIRMWARE=<FIRMWARE>.]))
        ])
)

#
# Scaling options
m4_include([m4/scaling-opts.m4])
# Scheduling options
m4_include([m4/scheduling-opts.m4])
# Realtime options
m4_include([m4/realtime-opts.m4])

# Checks for programs.
AC_PROG_CPP
AC_PROG_CC
AC_PROG_CXX
AM_PROG_AS
AC_PROG_RANLIB
AM_PROG_AR
AC_PROG_LN_S
AC_PROG_MKDIR_P
AC_PATH_PROG([TAR], [tar])
AS_IF([test -z "$TAR"], [
    AC_MSG_ERROR([Could not find 'tar' in PATH. Please install it or add it to PATH.])])
AC_PATH_PROG([CP], [cp])
AS_IF([test -z "$CP"], [
    AC_MSG_ERROR([Could not find 'cp' in PATH. Please install it or add it to PATH.])])
AC_PATH_PROG([TOUCH], [touch])
AS_IF([test -z "$TOUCH"], [
    AC_MSG_ERROR([Could not find 'touch' in PATH. Please install it or add it to PATH.])])

AM_CPPFLAGS=${ZBZ_CPP_FLAGS}
AM_CFLAGS="${ZBZ_C_CXX_COMMON_FLAGS} ${ZBZ_C_FLAGS}"
AM_CXXFLAGS="${ZBZ_C_CXX_COMMON_FLAGS} ${ZBZ_CXX_FLAGS}"
AM_LDFLAGS=${ZBZ_LD_FLAGS}

# Export host and build toolchain flags to be used in Makefile.am
AC_SUBST([ZARCH])
AC_SUBST([ZCHIPSET])
AC_SUBST([ZFIRMWARE])
AC_SUBST([ZSOC])

AC_SUBST([MAKE_IMM_PATH])

AC_SUBST([ZCHIPSET_COMMONLIBS_ENABLED_SUBDIRS])
AC_SUBST([ZCHIPSET_COMMONLIBS_KERNEL_LIBS])
AC_SUBST([ZCXXABI])

AC_SUBST([ZBZ_CPP_FLAGS])
AC_SUBST([ZBZ_C_FLAGS])
AC_SUBST([ZBZ_CXX_FLAGS])
AC_SUBST([ZBZ_LD_FLAGS])
AC_SUBST([AM_CPPFLAGS])
AC_SUBST([AM_CFLAGS])
AC_SUBST([AM_CXXFLAGS])
AC_SUBST([AM_LDFLAGS])

AC_SUBST([ISOPROG])
AC_SUBST([LN_S])
AC_SUBST([MKDIR_P])
AC_SUBST([TAR])
AC_SUBST([CP])
AC_SUBST([TOUCH])

#
# Distributary options.
#
DISTRIBUTARIES_ENABLED_SUBDIRS=""
m4_include([m4/distributary-opts.m4])
# Process DISTRIBUTARIES_ENABLED_SUBDIRS
DISTRIBUTARIES_KERNEL_LIBS=""
AS_IF([test -n "${DISTRIBUTARIES_ENABLED_SUBDIRS}"], [
    for dtrib in ${DISTRIBUTARIES_ENABLED_SUBDIRS}; do
        DISTRIBUTARIES_KERNEL_LIBS="${DISTRIBUTARIES_KERNEL_LIBS} __kcore/kernel/common/${dtrib}/lib${dtrib}.a"
    done
])
AC_SUBST([DISTRIBUTARIES_ENABLED_SUBDIRS])
AC_SUBST([DISTRIBUTARIES_KERNEL_LIBS])

#
# Debugging options.
#
m4_include([m4/debug-opts.m4])

#
# Itanium C++ ABI options.
#
m4_include([m4/icxxabi-opts.m4])

# Generate config.h and other files
AC_CONFIG_HEADERS([include/config.h])

# Output non-platform-specific Makefiles
AC_CONFIG_FILES([
    Makefile
    build/Makefile __kcore/Makefile
    __kcore/__kstdlib/Makefile
    __kcore/__kstdlib/__klibc/Makefile __kcore/__kstdlib/__klibcxx/Makefile
    __kcore/__kstdlib/icxxabi/Makefile
    __kcore/__kthreads/Makefile
    __kcore/platform/Makefile
    __kcore/chipset/Makefile
    __kcore/firmware/Makefile
    __kcore/arch/Makefile __kcore/kernel/Makefile
    __kcore/kernel/common/Makefile
    __kcore/kernel/common/cpuTrib/Makefile
    __kcore/kernel/common/debugTrib/Makefile __kcore/kernel/common/distributaryTrib/Makefile
    __kcore/kernel/common/execTrib/Makefile __kcore/kernel/common/floodplainn/Makefile
    __kcore/kernel/common/interruptTrib/Makefile __kcore/kernel/common/memoryTrib/Makefile
    __kcore/kernel/common/processTrib/Makefile __kcore/kernel/common/taskTrib/Makefile
    __kcore/kernel/common/timerTrib/Makefile __kcore/kernel/common/vfsTrib/Makefile
    __kcore/kernel/common/cisternn/Makefile
    __kcore/commonlibs/Makefile
    __kcore/commonlibs/drivers/Makefile __kcore/commonlibs/metalanguages/Makefile
    __kcore/commonlibs/libzbzcore/Makefile
    __kcore/__kclasses/Makefile
])

AS_IF([test "${ZARCH}" = "i586"], [
    AC_CONFIG_FILES([
        __kcore/__kstdlib/__klibc/i586/Makefile
        __kcore/__kthreads/i586/Makefile
        __kcore/arch/i586/Makefile
        __kcore/kernel/i586/Makefile
    ])
    AS_IF([test "${ZCHIPSET}" = "ibmPc"], [
        AC_CONFIG_FILES([
            __kcore/platform/i586-ibmPc/Makefile
            __kcore/commonlibs/libx86mp/Makefile
        ])
    ])
])

AS_IF([test "${ZCHIPSET}" = "ibmPc"], [
    AC_CONFIG_FILES([
        __kcore/chipset/ibmPc/Makefile
        __kcore/commonlibs/libacpi/Makefile
    ])
])

AS_IF([test "${ZFIRMWARE}" = "ibmPcBios"], [
    AC_CONFIG_FILES([
        __kcore/firmware/ibmPcBios/Makefile
    ])
])

m4_include([m4/ax_subdirs_configure.m4])

# Configure ZUDI tools to build native zudiindex.
AX_SUBDIRS_CONFIGURE([build/zudi],
    [[]],[[]],
    [[--build=${build_alias}],[--host=${build_alias}],[host_alias=${build_alias}]],
    [[host_alias=${host_alias}]])

AC_CONFIG_COMMANDS_POST(
    AC_MSG_NOTICE(m4_normalize(
        [Compiling Zambesii v${PACKAGE_VERSION} for platform:
        (Arch:${ZARCH})-(SoC:${ZSOC:-none})-(Chipset:${ZCHIPSET})-(Firmware:${ZFIRMWARE})]
    ))
    AC_MSG_NOTICE(m4_normalize(
        [Scaling options:
        (Mode:${ZSCALING}) (MAX CPUs:${ZMAX_NCPUS})]
    ))
    AC_MSG_NOTICE(m4_normalize(
        [Scheduler options:
        (Passive preemption:${enable_sched_preempt_passive})
        (Timeslice preemption:${enable_sched_preempt_timeslice})
        (Timeslice:${with_sched_preempt_timeslice_us}us)
        (Kernel preemption:${enable_sched_preempt_kernel})]
    ))
    AC_MSG_NOTICE(m4_normalize(
        [Realtime options:
        (Kernel IRQs:${enable_rt_kernel_irqs})
        (IRQ nesting:${enable_rt_kernel_irq_nesting})]
    ))
    AC_MSG_NOTICE(m4_normalize(
        [Debugging options:
        (Locks:${enable_debug_locks})
        (Scheduler:${enable_debug_scheduler})
        (Interrupts:${enable_debug_interrupts})
        (Page faults:${enable_debug_page_faults})]
    ))
    AC_MSG_NOTICE(m4_normalize(
        [Distributaries enabled:
        (${DISTRIBUTARIES_ENABLED_SUBDIRS})]
    ))
)

AC_OUTPUT
