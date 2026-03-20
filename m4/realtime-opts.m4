dnl Realtime options
dnl

dnl Enables local IRQ delivery while handling synchronous interrupt context
dnl such as page faults. The existing lock slow paths also use this to
dnl temporarily drop local IRQ masking while spinning.
AC_ARG_ENABLE([rt-sync-int-irqs],
    [AS_HELP_STRING([--enable-rt-sync-int-irqs],
        [Enable local IRQ delivery in sync-interrupt context and RT lock spin paths @<:@default=no@:>@])],
    [AS_CASE(["$enableval"],
        [no], [enable_rt_sync_int_irqs="no"],
        [yes|""|*], [enable_rt_sync_int_irqs="yes"])],
    [enable_rt_sync_int_irqs="no"]
)
AS_IF([test "$enable_rt_sync_int_irqs" = "yes"], [
    AC_DEFINE([CONFIG_RT_SYNC_INT_IRQS], [1], [Enable local IRQ delivery in sync-interrupt context and RT lock spin paths])
])

dnl Enables explicit nested IRQ handling by re-enabling local IRQs inside IRQ
dnl handlers after the handler has established its IRQ bookkeeping state.
AC_ARG_ENABLE([rt-sync-int-irq-nesting],
    [AS_HELP_STRING([--enable-rt-sync-int-irq-nesting],
        [Enable nested IRQ handling inside IRQ handlers @<:@default=no@:>@])],
    [AS_CASE(["$enableval"],
        [no], [enable_rt_sync_int_irq_nesting="no"],
        [yes|""|*], [enable_rt_sync_int_irq_nesting="yes"])],
    [enable_rt_sync_int_irq_nesting="no"]
)
AS_IF([test "$enable_rt_sync_int_irq_nesting" = "yes"], [
    enable_rt_sync_int_irqs="yes"
    AC_DEFINE([CONFIG_RT_SYNC_INT_IRQS], [1], [Enable local IRQ delivery in sync-interrupt context and RT lock spin paths])
    AC_DEFINE([CONFIG_RT_SYNC_INT_IRQ_NESTING], [1], [Enable nested IRQ handling inside IRQ handlers])
])
