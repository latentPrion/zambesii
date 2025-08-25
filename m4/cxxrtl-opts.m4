# C++ Runtime Library Constructor options configuration

# C++ global constructor packaging method configuration
AC_ARG_WITH([cxxrtl-ctor-packaging],
    [AS_HELP_STRING([--with-cxxrtl-ctor-packaging=@<:@ctors|init|init_array@:>@],
        [C++ global constructor packaging method @<:@default=auto-detect@:>@])],
    [AS_CASE(["$withval"],
        [ctors|.ctors|ctor|.ctor],
            [ctor_packaging="ctors_section"],
        [init|.init],
            [ctor_packaging="init_section"],
        [init_array|.init_array],
            [ctor_packaging="init_array_section"],
        [yes|*], [AC_MSG_ERROR(m4_normalize([Invalid packaging method: $withval.
                Valid options: ctors/.ctors/ctor/.ctor, init/.init,
                init_array/.init_array]))])],
	[]
)

# C++ global constructor call method configuration
AC_ARG_WITH([cxxrtl-ctor-callmethod],
    [AS_HELP_STRING(m4_normalize([--with-cxxrtl-ctor-callmethod=
        @<:@bounded|countnt|null_terminated|callable@:>@]),
        [C++ global constructor call method @<:@default=auto-detect@:>@])],
    [AS_CASE(["$withval"],
        [bounded|section],
            [ctor_call_method="bounded"],
        [countnt|count],
            [ctor_call_method="possibly_prefixed_count_null_terminated"],
        [nt|nullterm|nullterminated|null-terminated|null_terminated],
            [ctor_call_method="null_terminated"],
        [callable],
            [ctor_call_method="callable_section"],
        [yes|*], [AC_MSG_ERROR(m4_normalize([Invalid call method: $withval.
		Valid options: bounded/section, countnt/count,
		bounded/section, countnt/count,
                nt/nullterm/nullterminated/null-terminated/null_terminated,
                callable]))])],
	[]
)
