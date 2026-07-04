#
# IbmPcBios options
#
AC_ARG_WITH([debugpipe-device-bios-pxe-ethertype],
    [AS_HELP_STRING([--with-debugpipe-device-bios-pxe-ethertype=0xHEX],
        [Set custom EtherType for BIOS PXE debug pipe (default 0x02B2)])],
    [with_debugpipe_device_bios_pxe_ethertype="$withval"],
    [with_debugpipe_device_bios_pxe_ethertype="0x02B2"])

AC_ARG_WITH([debugpipe-device-bios-pxe-target-mac],
    [AS_HELP_STRING([--with-debugpipe-device-bios-pxe-target-mac=MAC],
        [Set destination MAC (aa:bb:cc:dd:ee:ff) for BIOS PXE debug pipe])],
    [with_debugpipe_device_bios_pxe_target_mac="$withval"],
    [with_debugpipe_device_bios_pxe_target_mac="ff:ff:ff:ff:ff:ff"])

AC_DEFINE_UNQUOTED([CONFIG_DEBUGPIPE_BIOS_PXE_ETHERTYPE],
    [$with_debugpipe_device_bios_pxe_ethertype],
    [Custom EtherType for BIOS PXE debug pipe])
AC_DEFINE_UNQUOTED([CONFIG_DEBUGPIPE_BIOS_PXE_TARGET_MAC],
    ["$with_debugpipe_device_bios_pxe_target_mac"],
    [Ethernet destination MAC for BIOS PXE debug pipe])
