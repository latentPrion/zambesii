dnl
dnl Kernel virtual address space options
dnl

dnl Kernel virtual address space demand paging option
AC_ARG_ENABLE([kernel-vaddrspace-demand-paging],
    [AS_HELP_STRING([--enable-kernel-vaddrspace-demand-paging],
        [Enable demand paging for kernel virtual address space @<:@default=no@:>@])],
    [AS_CASE(["$enableval"],
        [no], [enable_kernel_vaddrspace_demand_paging="no"],
        [yes|""|*], [enable_kernel_vaddrspace_demand_paging="yes"])],
    [enable_kernel_vaddrspace_demand_paging="no"]
)
AS_IF([test "$enable_kernel_vaddrspace_demand_paging" = "yes"], [
    AC_DEFINE([CONFIG_KERNEL_VADDRSPACE_DEMAND_PAGING], [1],
        [Enable demand paging for kernel virtual address space])
])

dnl
dnl Kernel heap options
dnl

dnl Heap demand paging option
AC_ARG_ENABLE([heap-demand-paging],
    [AS_HELP_STRING([--enable-heap-demand-paging],
        [Enable demand paging for kernel heap @<:@default=no@:>@])],
    [AS_CASE(["$enableval"],
        [no], [enable_heap_demand_paging="no"],
        [yes|""|*], [enable_heap_demand_paging="yes"])],
    [enable_heap_demand_paging="no"]
)
AS_IF([test "$enable_heap_demand_paging" = "yes"], [
    AC_DEFINE([CONFIG_HEAP_DEMAND_PAGING], [1],
        [Enable demand paging for kernel heap])
])

