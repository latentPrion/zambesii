dnl Scheduling options
dnl

dnl Passive preemption option
AC_ARG_ENABLE([sched-preempt-passive],
    [AS_HELP_STRING([--enable-sched-preempt-passive],
        [Enable passive preemption @<:@default=yes@:>@])],
    [AS_CASE(["$enableval"],
        [no], [enable_sched_preempt_passive="no"],
        [yes|""|*], [enable_sched_preempt_passive="yes"])],
    [enable_sched_preempt_passive="yes"]
)
AS_IF([test "$enable_sched_preempt_passive" = "yes"], [
    AC_DEFINE([CONFIG_SCHED_PREEMPT_PASSIVE], [1], [Enable passive preemption])
    AC_DEFINE([CONFIG_SCHED_PREEMPT], [1], [Enable scheduler preemption])
])

dnl Timeslice preemption option
AC_ARG_ENABLE([sched-preempt-timeslice],
    [AS_HELP_STRING([--enable-sched-preempt-timeslice],
        [Enable timeslice preemption @<:@default=yes@:>@])],
    [AS_CASE(["$enableval"],
        [no], [enable_sched_preempt_timeslice="no"],
        [yes|""|*], [enable_sched_preempt_timeslice="yes"])],
    [enable_sched_preempt_timeslice="no"]
)
AS_IF([test "$enable_sched_preempt_timeslice" = "yes"], [
    AC_DEFINE([CONFIG_SCHED_PREEMPT_TIMESLICE], [1], [Enable timeslice preemption])
    AC_DEFINE([CONFIG_SCHED_PREEMPT], [1], [Enable scheduler preemption])
])

dnl Timeslice duration option
AC_ARG_WITH([sched-preempt-timeslice-us],
    [AS_HELP_STRING([--with-sched-preempt-timeslice-us=MICROSECONDS],
        [Set scheduler timeslice duration in microseconds @<:@default=5000@:>@])],
    [], [
        AC_MSG_NOTICE([Using default timeslice duration: 5000us])
        with_sched_preempt_timeslice_us="5000"
    ]
)
ZBZ_VALIDATE_POSITIVE_NONZERO_INTEGER([with_sched_preempt_timeslice_us])
AC_DEFINE_UNQUOTED([CONFIG_SCHED_PREEMPT_TIMESLICE_US], [$with_sched_preempt_timeslice_us],
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
