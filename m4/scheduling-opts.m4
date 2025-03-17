dnl Scheduling options
dnl

dnl Preemption mode option
AC_ARG_ENABLE([sched-preemption],
    [AS_HELP_STRING([--enable-sched-preemption=MODE],
        [Set scheduler preemption mode: passive, timeslice, or no (cooperative) @<:@default=timeslice@:>@])],
    [AS_CASE(["$enableval"],
        [passive], [enable_sched_preemption="passive"],
        [no|coop|cooperative|co-op|co-operative], [enable_sched_preemption="coop"],
        # Default to timeslice
        [timeslice|timer|tick|""], [enable_sched_preemption="timeslice"],
        [*], [
            AC_MSG_ERROR([Invalid scheduler preemption mode: $enableval])
        ])],
    [enable_sched_preemption="timeslice"]
)
AS_CASE(["$enable_sched_preemption"],
    [coop], [
        AC_DEFINE([CONFIG_SCHED_COOP], [1],
            [Enable cooperative scheduling])],
    [passive], [
        AC_DEFINE([CONFIG_SCHED_PREEMPT_PASSIVE], [1],
            [Enable passive preemption])
        AC_DEFINE([CONFIG_SCHED_PREEMPT], [1],
            [Enable scheduler preemption])],
    [timeslice], [
        AC_DEFINE([CONFIG_SCHED_PREEMPT_TIMESLICE], [1],
            [Enable timeslice preemption (default)])
        AC_DEFINE([CONFIG_SCHED_PREEMPT], [1],
            [Enable scheduler preemption])],
    [*], [AC_MSG_ERROR(
        [Invalid scheduler preemption mode: $enable_sched_preemption])]
)

dnl Timeslice duration option
AC_ARG_WITH([sched-timeslice-us],
    [AS_HELP_STRING([--with-sched-timeslice-us=MICROSECONDS],
        [Set scheduler timeslice duration in microseconds @<:@default=5000@:>@])],
    [], [
        AC_MSG_NOTICE([Using default timeslice duration: 5000us])
        with_sched_timeslice_us="5000"
    ]
)
ZBZ_VALIDATE_POSITIVE_NONZERO_INTEGER([with_sched_timeslice_us])
AC_DEFINE_UNQUOTED([CONFIG_SCHED_TIMESLICE_US], [$with_sched_timeslice_us],
    [Set scheduler timeslice duration in microseconds])

dnl Kernel preemption option
AC_ARG_ENABLE([sched-preempt-kernel],
    [AS_HELP_STRING([--enable-sched-preempt-kernel],
        [Enable kernel preemption @<:@default=yes@:>@])],
    [AS_CASE(["$enable_sched_preempt_kernel"],
        [no], [ enable_sched_preempt_kernel="no" ],
        [yes|""|*], [ enable_sched_preempt_kernel="yes" ])],
    [enable_sched_preempt_kernel="yes"]
)
AS_IF([test "$enable_sched_preempt_kernel" = "yes"], [
    AC_DEFINE([CONFIG_SCHED_PREEMPT_KERNEL], [1], [Enable kernel preemption])
])
