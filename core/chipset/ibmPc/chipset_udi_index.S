
#include <in_asm.h>
#include <assembler.h>

.section .rodata

ASM_GLOBAL_DATA(chipset_udi_index_driver_headers)
.incbin "driver-headers.zudi-index"
ASM_END_DATA(chipset_udi_index_driver_headers)
ASM_GLOBAL_DATA(chipset_udi_index_driver_headers_end)

ASM_GLOBAL_DATA(chipset_udi_index_driver_data)
.incbin "driver-data.zudi-index"
ASM_END_DATA(chipset_udi_index_driver_data)
ASM_GLOBAL_DATA(chipset_udi_index_driver_data_end)

ASM_GLOBAL_DATA(chipset_udi_index_provisions)
.incbin "provisions.zudi-index"
ASM_END_DATA(chipset_udi_index_provisions)
ASM_GLOBAL_DATA(chipset_udi_index_provisions_end)

ASM_GLOBAL_DATA(chipset_udi_index_regions)
.incbin "regions.zudi-index"
ASM_END_DATA(chipset_udi_index_regions)
ASM_GLOBAL_DATA(chipset_udi_index_region_end)

ASM_GLOBAL_DATA(chipset_udi_index_devices)
.incbin "devices.zudi-index"
ASM_END_DATA(chipset_udi_index_devices)
ASM_GLOBAL_DATA(chipset_udi_index_devices_end)

ASM_GLOBAL_DATA(chipset_udi_index_ranks)
.incbin "ranks.zudi-index"
ASM_END_DATA(chipset_udi_index_ranks)
ASM_GLOBAL_DATA(chipset_udi_index_ranks_end)

ASM_GLOBAL_DATA(chipset_udi_index_messages)
.incbin "messages.zudi-index"
ASM_END_DATA(chipset_udi_index_messages)
ASM_GLOBAL_DATA(chipset_udi_index_messages_end)

ASM_GLOBAL_DATA(chipset_udi_index_disaster_messages)
.incbin "disaster-messages.zudi-index"
ASM_END_DATA(chipset_udi_index_disaster_messages)
ASM_GLOBAL_DATA(chipset_udi_index_disaster_messages_end)

ASM_GLOBAL_DATA(chipset_udi_index_message_files)
.incbin "message-files.zudi-index"
ASM_END_DATA(chipset_udi_index_message_files)
ASM_GLOBAL_DATA(chipset_udi_index_message_files_end)

ASM_GLOBAL_DATA(chipset_udi_index_readable_files)
.incbin "readable-files.zudi-index"
ASM_END_DATA(chipset_udi_index_readable_files)
ASM_GLOBAL_DATA(chipset_udi_index_readable_files_end)

