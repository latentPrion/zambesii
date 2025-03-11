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
