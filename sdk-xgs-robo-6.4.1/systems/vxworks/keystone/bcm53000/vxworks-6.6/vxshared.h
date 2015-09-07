/*
 * VXWorks-specific driver data structure fields and defines
 *
 * Copyright(c) 2003 Broadcom Corporation
 * $Id: vxshared.h,v 1.3 2010/01/23 08:16:33 miyarn Exp $
 *
 */
#ifndef	_VXSHARED_H
#define	_VXSHARED_H

/* vx def's used by vx_osl.c */
#ifndef ROBODVT
#define	VXCLSIZE	2044	/* Cluster size */
#else
#define VXCLSIZE        16318   /* Cluster size */
#endif
#define	M_HEADROOM(m)	((m)->mBlkHdr.mData - (m)->pClBlk->clNode.pClBuf)

/* Tornado 2.1 pciClass.h definitions */
#ifndef __INCpciClassh
#define __INCpciClassh
#define PCI_CLASS_NETWORK_CTLR	0x02	/* Class for network controller */
#define PCI_SUBCLASS_00         0x00	/* Subclass 0x00 for ethernet */
#define PCI_SUBCLASS_80         0x80	/* Subclass 0x80 for other */
#define PCI_SUBCLASS_NET_ETHERNET	(PCI_SUBCLASS_00)
#define PCI_SUBCLASS_NET_OTHER		(PCI_SUBCLASS_80)
#endif /* __INCpciClassh */

#endif /* _VXSHARED_H */
