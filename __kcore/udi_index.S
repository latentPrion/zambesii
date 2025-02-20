
#include <in_asm.h>
#include <assembler.h>

ASM_SECTION(.rodata)

ASM_GLOBAL_DATA(__kudi_index_drivers)
.incbin "drivers.zudi-index"
ASM_END_DATA(__kudi_index_drivers)
ASM_GLOBAL_DATA(__kudi_index_drivers_end)

ASM_GLOBAL_DATA(__kudi_index_data)
.incbin "data.zudi-index"
ASM_END_DATA(__kudi_index_data)
ASM_GLOBAL_DATA(__kudi_index_data_end)

ASM_GLOBAL_DATA(__kudi_index_provisions)
.incbin "provisions.zudi-index"
ASM_END_DATA(__kudi_index_provisions)
ASM_GLOBAL_DATA(__kudi_index_provisions_end)

ASM_GLOBAL_DATA(__kudi_index_devices)
.incbin "devices.zudi-index"
ASM_END_DATA(__kudi_index_devices)
ASM_GLOBAL_DATA(__kudi_index_devices_end)

ASM_GLOBAL_DATA(__kudi_index_ranks)
.incbin "ranks.zudi-index"
ASM_END_DATA(__kudi_index_ranks)
ASM_GLOBAL_DATA(__kudi_index_ranks_end)

ASM_GLOBAL_DATA(__kudi_index_strings)
.incbin "strings.zudi-index"
ASM_END_DATA(__kudi_index_strings)
ASM_GLOBAL_DATA(__kudi_index_strings_end)

