
#include <commonlibs/libx86mp/mpDefaultTables.h>
#include <commonlibs/libx86mp/mpTables.h>

/**	EXPLANATION:
 * Hardcoded tables for the default configurations on Intel MP.
 **/

namespace x86Mp
{
	struct sConfig1
	{
		struct sConfig		cfg;
		struct sCpuConfig	cpus[2];
		struct x86Mp::sIoApicConfig	ioApic;
		// We won't care about the IRQ Source entries anytime soon.
	};

	static struct sConfig1	cfg1 =
	{
		{
			{'P', 'C', 'M', 'P'},
			sizeof(cfg1),
			0x01,
			// Checksum.
			0,
			{'Z', 'a', 'm', 'b', 'e', 's', 'i', 'i'},
			{'Z', 'a', 'm', 'b', 'e', 's', 'i', 'i', ' ', ' ', ' ', ' '},
			0x0,
			0x0,
			// nEntries.
			0x3,
			0xFEC00000,
			0x0,
			0x0,
			0x0
		},
		{
			{
				x86_MPCFG_TYPE_CPU,
				0x0,
				// FIXME: LAPIC Version. Check over.
				0x01,
				x86_MPCFG_CPU_FLAGS_BSP | x86_MPCFG_CPU_FLAGS_ENABLED,
				0x0,
				0x0, 0x0, 0x0
			},
			{
				x86_MPCFG_TYPE_CPU,
				0x1,
				// FIXME: LAPIC Version. Check over.
				0x01,
				x86_MPCFG_CPU_FLAGS_ENABLED,
				0x0,
				0x0, 0x0, 0x0
			}
		},
		{
			x86_MPCFG_TYPE_IOAPIC,
			0x2,
			// FIXME: I/O APIC Version. Check over values.
			0x1,
			x86_MPCFG_IOAPIC_FLAGS_ENABLED,
			0xFEE00000
		}

	};

	static struct sConfig1	cfg2 =
	{
		{
			{'P', 'C', 'M', 'P'},
			sizeof(cfg2),
			0x01,
			// Checksum.
			0,
			{'Z', 'a', 'm', 'b', 'e', 's', 'i', 'i'},
			{'Z', 'a', 'm', 'b', 'e', 's', 'i', 'i', ' ', ' ', ' ', ' '},
			0x0,
			0x0,
			// nEntries.
			0x3,
			0xFEC00000,
			0x0,
			0x0,
			0x0
		},
		{
			{
				x86_MPCFG_TYPE_CPU,
				0x0,
				// FIXME: LAPIC Version. Check over.
				0x01,
				x86_MPCFG_CPU_FLAGS_BSP | x86_MPCFG_CPU_FLAGS_ENABLED,
				0x0,
				0x0, 0x0, 0x0
			},
			{
				x86_MPCFG_TYPE_CPU,
				0x1,
				// FIXME: LAPIC Version. Check over.
				0x01,
				x86_MPCFG_CPU_FLAGS_ENABLED,
				0x0,
				0x0, 0x0, 0x0
			}
		},
		{
			x86_MPCFG_TYPE_IOAPIC,
			0x2,
			// FIXME: I/O APIC Version. Check over values.
			0x1,
			x86_MPCFG_IOAPIC_FLAGS_ENABLED,
			0xFEE00000
		}

	};

	static struct sConfig1	cfg3 =
	{
		{
			{'P', 'C', 'M', 'P'},
			sizeof(cfg3),
			0x01,
			// Checksum.
			0,
			{'Z', 'a', 'm', 'b', 'e', 's', 'i', 'i'},
			{'Z', 'a', 'm', 'b', 'e', 's', 'i', 'i', ' ', ' ', ' ', ' '},
			0x0,
			0x0,
			// nEntries.
			0x3,
			0xFEC00000,
			0x0,
			0x0,
			0x0
		},
		{
			{
				x86_MPCFG_TYPE_CPU,
				0x0,
				// FIXME: LAPIC Version. Check over.
				0x01,
				x86_MPCFG_CPU_FLAGS_BSP | x86_MPCFG_CPU_FLAGS_ENABLED,
				0x0,
				0x0, 0x0, 0x0
			},
			{
				x86_MPCFG_TYPE_CPU,
				0x1,
				// FIXME: LAPIC Version. Check over.
				0x01,
				x86_MPCFG_CPU_FLAGS_ENABLED,
				0x0,
				0x0, 0x0, 0x0
			}
		},
		{
			x86_MPCFG_TYPE_IOAPIC,
			0x2,
			// FIXME: I/O APIC Version. Check over values.
			0x1,
			x86_MPCFG_IOAPIC_FLAGS_ENABLED,
			0xFEE00000
		}

	};

	static struct sConfig1	cfg4 =
	{
		{
			{'P', 'C', 'M', 'P'},
			sizeof(cfg4),
			0x01,
			// Checksum.
			0,
			{'Z', 'a', 'm', 'b', 'e', 's', 'i', 'i'},
			{'Z', 'a', 'm', 'b', 'e', 's', 'i', 'i', ' ', ' ', ' ', ' '},
			0x0,
			0x0,
			// nEntries.
			0x3,
			0xFEC00000,
			0x0,
			0x0,
			0x0
		},
		{
			{
				x86_MPCFG_TYPE_CPU,
				0x0,
				// FIXME: LAPIC Version. Check over.
				0x01,
				x86_MPCFG_CPU_FLAGS_BSP | x86_MPCFG_CPU_FLAGS_ENABLED,
				0x0,
				0x0, 0x0, 0x0
			},
			{
				x86_MPCFG_TYPE_CPU,
				0x1,
				// FIXME: LAPIC Version. Check over.
				0x01,
				x86_MPCFG_CPU_FLAGS_ENABLED,
				0x0,
				0x0, 0x0, 0x0
			}
		},
		{
			x86_MPCFG_TYPE_IOAPIC,
			0x2,
			// FIXME: I/O APIC Version. Check over values.
			0x1,
			x86_MPCFG_IOAPIC_FLAGS_ENABLED,
			0xFEE00000
		}

	};

	static struct sConfig1	cfg5 =
	{
		{
			{'P', 'C', 'M', 'P'},
			sizeof(cfg5),
			0x01,
			// Checksum.
			0,
			{'Z', 'a', 'm', 'b', 'e', 's', 'i', 'i'},
			{'Z', 'a', 'm', 'b', 'e', 's', 'i', 'i', ' ', ' ', ' ', ' '},
			0x0,
			0x0,
			// nEntries.
			0x3,
			0xFEC00000,
			0x0,
			0x0,
			0x0
		},
		{
			{
				x86_MPCFG_TYPE_CPU,
				0x0,
				// FIXME: LAPIC Version. Check over.
				0x01,
				x86_MPCFG_CPU_FLAGS_BSP | x86_MPCFG_CPU_FLAGS_ENABLED,
				0x0,
				0x0, 0x0, 0x0
			},
			{
				x86_MPCFG_TYPE_CPU,
				0x1,
				// FIXME: LAPIC Version. Check over.
				0x01,
				x86_MPCFG_CPU_FLAGS_ENABLED,
				0x0,
				0x0, 0x0, 0x0
			}
		},
		{
			x86_MPCFG_TYPE_IOAPIC,
			0x2,
			// FIXME: I/O APIC Version. Check over values.
			0x1,
			x86_MPCFG_IOAPIC_FLAGS_ENABLED,
			0xFEE00000
		}

	};

	static struct sConfig1	cfg6 =
	{
		{
			{'P', 'C', 'M', 'P'},
			sizeof(cfg6),
			0x01,
			// Checksum.
			0,
			{'Z', 'a', 'm', 'b', 'e', 's', 'i', 'i'},
			{'Z', 'a', 'm', 'b', 'e', 's', 'i', 'i', ' ', ' ', ' ', ' '},
			0x0,
			0x0,
			// nEntries.
			0x3,
			0xFEC00000,
			0x0,
			0x0,
			0x0
		},
		{
			{
				x86_MPCFG_TYPE_CPU,
				0x0,
				// FIXME: LAPIC Version. Check over.
				0x01,
				x86_MPCFG_CPU_FLAGS_BSP | x86_MPCFG_CPU_FLAGS_ENABLED,
				0x0,
				0x0, 0x0, 0x0
			},
			{
				x86_MPCFG_TYPE_CPU,
				0x1,
				// FIXME: LAPIC Version. Check over.
				0x01,
				x86_MPCFG_CPU_FLAGS_ENABLED,
				0x0,
				0x0, 0x0, 0x0
			}
		},
		{
			x86_MPCFG_TYPE_IOAPIC,
			0x2,
			// FIXME: I/O APIC Version. Check over values.
			0x1,
			x86_MPCFG_IOAPIC_FLAGS_ENABLED,
			0xFEE00000
		}

	};

	static struct sConfig1	cfg7 =
	{
		{
			{'P', 'C', 'M', 'P'},
			sizeof(cfg7),
			0x01,
			// Checksum.
			0,
			{'Z', 'a', 'm', 'b', 'e', 's', 'i', 'i'},
			{'Z', 'a', 'm', 'b', 'e', 's', 'i', 'i', ' ', ' ', ' ', ' '},
			0x0,
			0x0,
			// nEntries.
			0x3,
			0xFEC00000,
			0x0,
			0x0,
			0x0
		},
		{
			{
				x86_MPCFG_TYPE_CPU,
				0x0,
				// FIXME: LAPIC Version. Check over.
				0x01,
				x86_MPCFG_CPU_FLAGS_BSP | x86_MPCFG_CPU_FLAGS_ENABLED,
				0x0,
				0x0, 0x0, 0x0
			},
			{
				x86_MPCFG_TYPE_CPU,
				0x1,
				// FIXME: LAPIC Version. Check over.
				0x01,
				x86_MPCFG_CPU_FLAGS_ENABLED,
				0x0,
				0x0, 0x0, 0x0
			}
		},
		{
			x86_MPCFG_TYPE_IOAPIC,
			0x2,
			// FIXME: I/O APIC Version. Check over values.
			0x1,
			x86_MPCFG_IOAPIC_FLAGS_ENABLED,
			0xFEE00000
		}

	};

	struct sConfig		*configDefaults[7] =
	{
		(struct sConfig *)&cfg1,
		(struct sConfig *)&cfg2,
		(struct sConfig *)&cfg3,
		(struct sConfig *)&cfg4,
		(struct sConfig *)&cfg5,
		(struct sConfig *)&cfg6,
		(struct sConfig *)&cfg7
	};
}
