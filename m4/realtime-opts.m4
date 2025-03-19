dnl Realtime options
dnl

dnl Enables IRQs to come in when a CPU is executing in kernel mode.
dnl This allows for better real-time responsiveness by not masking interrupts
dnl during kernel execution, at the cost of increased complexity.
AC_ARG_ENABLE([rt-kernel-irqs],
    [AS_HELP_STRING([--enable-rt-kernel-irqs],
        [Enable IRQs during kernel execution for better real-time responsiveness @<:@default=no@:>@])],
    [AS_CASE(["$enableval"],
        [no], [enable_rt_kernel_irqs="no"],
        [yes|""|*], [enable_rt_kernel_irqs="yes"])],
    [enable_rt_kernel_irqs="no"]
)
AS_IF([test "$enable_rt_kernel_irqs" = "yes"], [
    AC_DEFINE([CONFIG_RT_KERNEL_IRQS], [1], [Enable IRQs during kernel execution for real-time responsiveness])
])

dnl Enables Kernel-mode IRQs to nest.
dnl This allows higher priority interrupts to preempt lower priority ones
dnl even when already handling an interrupt, improving real-time responsiveness.
AC_ARG_ENABLE([rt-kernel-irq-nesting],
    [AS_HELP_STRING([--enable-rt-kernel-irq-nesting],
        [Enable nested IRQs in kernel mode for better real-time responsiveness @<:@default=no@:>@])],
    [AS_CASE(["$enableval"],
        [no], [enable_rt_kernel_irq_nesting="no"],
        [yes|""|*], [enable_rt_kernel_irq_nesting="yes"])],
    [enable_rt_kernel_irq_nesting="no"]
)
AS_IF([test "$enable_rt_kernel_irq_nesting" = "yes"], [
    AC_DEFINE([CONFIG_RT_KERNEL_IRQS], [1], [Enable IRQs during kernel execution for real-time responsiveness])
    AC_DEFINE([CONFIG_RT_KERNEL_IRQ_NESTING], [1], [Enable nested IRQs in kernel mode for better real-time responsiveness])
])
