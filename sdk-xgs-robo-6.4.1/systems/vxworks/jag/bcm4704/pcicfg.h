/*
 * pcicfg.h: PCI configuration  constants and structures.
 *
 * Copyright (c) 1997 Epigram, Inc.
 *
 * $Id: pcicfg.h,v 1.1 2004/02/24 07:47:00 csm Exp $
 */

#ifndef	_h_pci_
#define	_h_pci_

/* The following inside ifndef's so we don't collide with NTDDK.H */
#ifndef PCI_MAX_BUS
#define PCI_MAX_BUS		0x100
#endif
#ifndef PCI_MAX_DEVICES
#define PCI_MAX_DEVICES		0x20
#endif
#ifndef PCI_MAX_FUNCTION
#define PCI_MAX_FUNCTION	0x8
#endif

#ifndef PCI_INVALID_VENDORID
#define PCI_INVALID_VENDORID	0xffff
#endif
#ifndef PCI_INVALID_DEVICEID
#define PCI_INVALID_DEVICEID	0xffff
#endif


/* Convert between bus-slot-function-register and config addresses */

#define	PCICFG_BUS_SHIFT	16	/* Bus shift */
#define	PCICFG_SLOT_SHIFT	11	/* Slot shift */
#define	PCICFG_FUN_SHIFT	8	/* Function shift */
#define	PCICFG_OFF_SHIFT	0	/* Bus shift */

#define	PCICFG_BUS_MASK		0xff	/* Bus mask */
#define	PCICFG_SLOT_MASK	0x1f	/* Slot mask */
#define	PCICFG_FUN_MASK		7	/* Function mask */
#define	PCICFG_OFF_MASK		0xff	/* Bus mask */

#define	PCI_CONFIG_ADDR(b, s, f, o)					\
		((((b) & PCICFG_BUS_MASK) << PCICFG_BUS_SHIFT)		\
		 | (((s) & PCICFG_SLOT_MASK) << PCICFG_SLOT_SHIFT)	\
		 | (((f) & PCICFG_FUN_MASK) << PCICFG_FUN_SHIFT)	\
		 | (((o) & PCICFG_OFF_MASK) << PCICFG_OFF_SHIFT))

#define	PCI_CONFIG_BUS(a)	(((a) >> PCICFG_BUS_SHIFT) & PCICFG_BUS_MASK)
#define	PCI_CONFIG_SLOT(a)	(((a) >> PCICFG_SLOT_SHIFT) & PCICFG_SLOT_MASK)
#define	PCI_CONFIG_FUN(a)	(((a) >> PCICFG_FUN_SHIFT) & PCICFG_FUN_MASK)
#define	PCI_CONFIG_OFF(a)	(((a) >> PCICFG_OFF_SHIFT) & PCICFG_OFF_MASK)


/* The actual config space */

#define	PCI_BAR_MAX		6

#define	PCI_ROM_BAR		8

#define	PCR_RSVDA_MAX		2

typedef struct _pci_config_regs {
    unsigned short	vendor;
    unsigned short	device;
    unsigned short	command;
    unsigned short	status;
    unsigned char	rev_id;
    unsigned char	prog_if;
    unsigned char	sub_class;
    unsigned char	base_class;
    unsigned char	cache_line_size;
    unsigned char	latency_timer;
    unsigned char	header_type;
    unsigned char	bist;
    unsigned long	base[PCI_BAR_MAX];
    unsigned long	cardbus_cis;
    unsigned short	subsys_vendor;
    unsigned short	subsys_id;
    unsigned long	baserom;
    unsigned long	rsvd_a[PCR_RSVDA_MAX];
    unsigned char	int_line;
    unsigned char	int_pin;
    unsigned char	min_gnt;
    unsigned char	max_lat;
    unsigned char	dev_dep[192];
} pci_config_regs;

#define	SZPCR		(sizeof (pci_config_regs))
#define	MINSZPCR	64		/* offsetof (dev_dep[0] */

/* A structure for the config registers is nice, but in most
 * systems the config space is not memory mapped, so we need
 * filed offsetts. :-(
 */
#define	PCI_CFG_VID		0
#define	PCI_CFG_DID		2
#define	PCI_CFG_CMD		4
#define	PCI_CFG_STAT		6
#define	PCI_CFG_REV		8
#define	PCI_CFG_PROGIF		9
#define	PCI_CFG_SUBCL		0xa
#define	PCI_CFG_BASECL		0xb
#define	PCI_CFG_CLSZ		0xc
#define	PCI_CFG_LATTIM		0xd
#define	PCI_CFG_HDR		0xe
#define	PCI_CFG_BIST		0xf
#define	PCI_CFG_BAR0		0x10
#define	PCI_CFG_BAR1		0x14
#define	PCI_CFG_BAR2		0x18
#define	PCI_CFG_BAR3		0x1c
#define	PCI_CFG_BAR4		0x20
#define	PCI_CFG_BAR5		0x24
#define	PCI_CFG_CIS		0x28
#define	PCI_CFG_SVID		0x2c
#define	PCI_CFG_SSID		0x2e
#define	PCI_CFG_ROMBAR		0x30
#define	PCI_CFG_INT		0x3c
#define	PCI_CFG_PIN		0x3d
#define	PCI_CFG_MINGNT		0x3e
#define	PCI_CFG_MAXLAT		0x3f

/* Classes and subclasses */

typedef enum {
    PCI_CLASS_OLD = 0,
    PCI_CLASS_DASDI,
    PCI_CLASS_NET,
    PCI_CLASS_DISPLAY,
    PCI_CLASS_MMEDIA,
    PCI_CLASS_MEMORY,
    PCI_CLASS_BRIDGE,
    PCI_CLASS_COMM,
    PCI_CLASS_BASE,
    PCI_CLASS_INPUT,
    PCI_CLASS_DOCK,
    PCI_CLASS_CPU,
    PCI_CLASS_SERIAL,
    PCI_CLASS_INTELLIGENT = 0xe,
    PCI_CLASS_SATELLITE,
    PCI_CLASS_CRYPT,
    PCI_CLASS_DSP,
    PCI_CLASS_MAX
} pci_classes;

typedef enum {
    PCI_DASDI_SCSI,
    PCI_DASDI_IDE,
    PCI_DASDI_FLOPPY,
    PCI_DASDI_IPI,
    PCI_DASDI_RAID,
    PCI_DASDI_OTHER = 0x80
} pci_dasdi_subclasses;

typedef enum {
    PCI_NET_ETHER,
    PCI_NET_TOKEN,
    PCI_NET_FDDI,
    PCI_NET_ATM,
    PCI_NET_OTHER = 0x80
} pci_net_subclasses;

typedef enum {
    PCI_DISPLAY_VGA,
    PCI_DISPLAY_XGA,
    PCI_DISPLAY_3D,
    PCI_DISPLAY_OTHER = 0x80
} pci_display_subclasses;

typedef enum {
    PCI_MMEDIA_VIDEO,
    PCI_MMEDIA_AUDIO,
    PCI_MMEDIA_PHONE,
    PCI_MEDIA_OTHER = 0x80
} pci_mmedia_subclasses;

typedef enum {
    PCI_MEMORY_RAM,
    PCI_MEMORY_FLASH,
    PCI_MEMORY_OTHER = 0x80
} pci_memory_subclasses;

typedef enum {
    PCI_BRIDGE_HOST,
    PCI_BRIDGE_ISA,
    PCI_BRIDGE_EISA,
    PCI_BRIDGE_MC,
    PCI_BRIDGE_PCI,
    PCI_BRIDGE_PCMCIA,
    PCI_BRIDGE_NUBUS,
    PCI_BRIDGE_CARDBUS,
    PCI_BRIDGE_RACEWAY,
    PCI_BRIDGE_OTHER = 0x80
} pci_bridge_subclasses;

typedef enum {
    PCI_COMM_UART,
    PCI_COMM_PARALLEL,
    PCI_COMM_MULTIUART,
    PCI_COMM_MODEM,
    PCI_COMM_OTHER = 0x80
} pci_comm_subclasses;

typedef enum {
    PCI_BASE_PIC,
    PCI_BASE_DMA,
    PCI_BASE_TIMER,
    PCI_BASE_RTC,
    PCI_BASE_PCI_HOTPLUG,
    PCI_BASE_OTHER = 0x80
} pci_base_subclasses;

typedef enum {
    PCI_INPUT_KBD,
    PCI_INPUT_PEN,
    PCI_INPUT_MOUSE,
    PCI_INPUT_SCANNER,
    PCI_INPUT_GAMEPORT,
    PCI_INPUT_OTHER = 0x80
} pci_input_subclasses;

typedef enum {
    PCI_DOCK_GENERIC,
    PCI_DOCK_OTHER = 0x80
} pci_dock_subclasses;

typedef enum {
    PCI_CPU_386,
    PCI_CPU_486,
    PCI_CPU_PENTIUM,
    PCI_CPU_ALPHA = 0x10,
    PCI_CPU_POWERPC = 0x20,
    PCI_CPU_MIPS = 0x30,
    PCI_CPU_COPROC = 0x40,
    PCI_CPU_OTHER = 0x80
} pci_cpu_subclasses;

typedef enum {
    PCI_SERIAL_IEEE1394,
    PCI_SERIAL_ACCESS,
    PCI_SERIAL_SSA,
    PCI_SERIAL_USB,
    PCI_SERIAL_FIBER,
    PCI_SERIAL_SMBUS,
    PCI_SERIAL_OTHER = 0x80
} pci_serial_subclasses;

typedef enum {
    PCI_INTELLIGENT_I2O,
} pci_intelligent_subclasses;

typedef enum {
    PCI_SATELLITE_TV,
    PCI_SATELLITE_AUDIO,
    PCI_SATELLITE_VOICE,
    PCI_SATELLITE_DATA,
    PCI_SATELLITE_OTHER = 0x80
} pci_satellite_subclasses;

typedef enum {
    PCI_CRYPT_NETWORK,
    PCI_CRYPT_ENTERTAINMENT,
    PCI_CRYPT_OTHER = 0x80
} pci_crypt_subclasses;

typedef enum {
    PCI_DSP_DPIO,
    PCI_DSP_OTHER = 0x80
} pci_dsp_subclasses;

/* Header types */
typedef enum {
	PCI_HEADER_NORMAL,
	PCI_HEADER_BRIDGE,
	PCI_HEADER_CARDBUS
} pci_header_types;


/* Overlay for a PCI-to-PCI bridge */

#define	PPB_RSVDA_MAX		2
#define	PPB_RSVDD_MAX		8

typedef struct _ppb_config_regs {
    unsigned short	vendor;
    unsigned short	device;
    unsigned short	command;
    unsigned short	status;
    unsigned char	rev_id;
    unsigned char	prog_if;
    unsigned char	sub_class;
    unsigned char	base_class;
    unsigned char	cache_line_size;
    unsigned char	latency_timer;
    unsigned char	header_type;
    unsigned char	bist;
    unsigned long	rsvd_a[PPB_RSVDA_MAX];
    unsigned char	prim_bus;
    unsigned char	sec_bus;
    unsigned char	sub_bus;
    unsigned char	sec_lat;
    unsigned char	io_base;
    unsigned char	io_lim;
    unsigned short	sec_status;
    unsigned short	mem_base;
    unsigned short	mem_lim;
    unsigned short	pf_mem_base;
    unsigned short	pf_mem_lim;
    unsigned long	pf_mem_base_hi;
    unsigned long	pf_mem_lim_hi;
    unsigned short	io_base_hi;
    unsigned short	io_lim_hi;
    unsigned short	subsys_vendor;
    unsigned short	subsys_id;
    unsigned long	rsvd_b;
    unsigned char	rsvd_c;
    unsigned char	int_pin;
    unsigned short	bridge_ctrl;
    unsigned char	chip_ctrl;
    unsigned char	diag_ctrl;
    unsigned short	arb_ctrl;
    unsigned long	rsvd_d[PPB_RSVDD_MAX];
    unsigned char	dev_dep[192];
} ppb_config_regs;

/* pci config registers */
#define	PCI_BAR0_WIN		0x80
#define	PCI_BAR1_WIN		0x84
#define	PCI_SPROM_CONTROL	0x88
#define	PCI_BAR1_CONTROL	0x8c
#define	PCI_INT_STATUS		0x90
#define	PCI_INT_MASK		0x94

#define	SBIM_SHIFT		8	/* pciintmask */
#define	SBIM_MASK		0xff00

#define	SPROM_BLANK		0x04  	/* sprom control bit indicating a blank sprom */
#define SPROM_WRITEEN		0x10	/* sprom write enable */

#define	SPROM_SIZE			256		/* sprom size in 16-bit */
#define SPROM_CRC_RANGE		64		/* crc cover range in 16-bit */

#define	PCI_GPIO_IN		0xb0	/* pci config space gpio input (>=rev3) */
#define	PCI_GPIO_OUT		0xb4	/* pci config space gpio output (>=rev3) */
#define	PCI_GPIO_OUTEN		0xb8	/* pci config space gpio output enable (>=rev3) */

#define	PCI_BAR0_SPROM_OFFSET	(4 * 1024)	/* bar0 + 4K accesses external sprom */
#define	PCI_BAR0_PCIREGS_OFFSET	(6 * 1024)	/* bar0 + 6K accesses pci core registers */

#endif
