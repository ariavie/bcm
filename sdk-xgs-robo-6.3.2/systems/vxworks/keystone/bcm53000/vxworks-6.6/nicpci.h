/*
 * BCM43XX PCI/E core sw API definitions.
 *
 * $Copyright: Copyright 2012 Broadcom Corporation.
 * This program is the proprietary software of Broadcom Corporation
 * and/or its licensors, and may only be used, duplicated, modified
 * or distributed pursuant to the terms and conditions of a separate,
 * written license agreement executed between you and Broadcom
 * (an "Authorized License").  Except as set forth in an Authorized
 * License, Broadcom grants no license (express or implied), right
 * to use, or waiver of any kind with respect to the Software, and
 * Broadcom expressly reserves all rights in and to the Software
 * and all intellectual property rights therein.  IF YOU HAVE
 * NO AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE
 * IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE
 * ALL USE OF THE SOFTWARE.  
 *  
 * Except as expressly set forth in the Authorized License,
 *  
 * 1.     This program, including its structure, sequence and organization,
 * constitutes the valuable trade secrets of Broadcom, and you shall use
 * all reasonable efforts to protect the confidentiality thereof,
 * and to use this information only in connection with your use of
 * Broadcom integrated circuit products.
 *  
 * 2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS
 * PROVIDED "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
 * REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY,
 * OR OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
 * DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
 * NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
 * ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
 * CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING
 * OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
 * 
 * 3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL
 * BROADCOM OR ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL,
 * INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER
 * ARISING OUT OF OR IN ANY WAY RELATING TO YOUR USE OF OR INABILITY
 * TO USE THE SOFTWARE EVEN IF BROADCOM HAS BEEN ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES; OR (ii) ANY AMOUNT IN EXCESS OF
 * THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF OR USD 1.00,
 * WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING
 * ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.$
 *
 * $Id: nicpci.h 1.3 Broadcom SDK $
 */

#ifndef	_NICPCI_H
#define	_NICPCI_H

#if defined(BCMSDIO) || defined(BCMDHDUSB) || (defined(BCMBUSTYPE) && (BCMBUSTYPE == SI_BUS))
#define pcicore_find_pci_capability(a, b, c, d) (0)
#define pcie_readreg(a, b, c, d) (0)
#define pcie_writereg(a, b, c, d, e) (0)

#define pcie_clkreq(a, b, c) (0)

#define pcicore_init(a, b, c) (0x0dadbeef)
#define pcicore_deinit(a)	do { } while (0)
#define pcicore_attach(a, b, c)	do { } while (0)
#define pcicore_hwup(a)		do { } while (0)
#define pcicore_up(a, b)	do { } while (0)
#define pcicore_sleep(a)	do { } while (0)
#define pcicore_down(a, b)	do { } while (0)

#define pcie_war_ovr_aspm_disable(a)	do { } while (0)

#ifdef BCMINTERNAL
#define pcicore_pciereg(a, b, c, d, e) (0)
#define pcicore_pcieserdesreg(a, b, c, d, e) (0)
#ifdef BCMDBG
#define pcicore_dump_pcieregs(a, b) (0)
#endif
#endif /* BCMINTERNAL */
#ifdef BCMDBG
#define pcie_lcreg(a, b, c) (0)
#define pcicore_dump(a, b)	do { } while (0)
#endif

#define pcicore_pmecap_fast(a)	(FALSE)
#define pcicore_pmeen(a)	do { } while (0)
#define pcicore_pmeclr(a)	(FALSE)
#else
struct sbpcieregs;

extern uint8 pcicore_find_pci_capability(osl_t *osh, uint8 req_cap_id,
                                         uchar *buf, uint32 *buflen);
extern uint pcie_readreg(osl_t *osh, struct sbpcieregs *pcieregs, uint addrtype, uint offset);
extern uint pcie_writereg(osl_t *osh, struct sbpcieregs *pcieregs, uint addrtype, uint offset,
                          uint val);

extern uint8 pcie_clkreq(void *pch, uint32 mask, uint32 val);

extern void *pcicore_init(si_t *sih, osl_t *osh, void *regs);
extern void pcicore_deinit(void *pch);
extern void pcicore_attach(void *pch, char *pvars, int state);
extern void pcicore_hwup(void *pch);
extern void pcicore_up(void *pch, int state);
extern void pcicore_sleep(void *pch);
extern void pcicore_down(void *pch, int state);

extern void pcie_war_ovr_aspm_disable(void *pch);

#ifdef BCMINTERNAL
extern uint32 pcicore_pciereg(void *pch, uint32 offset, uint32 mask, uint32 val, uint type);
extern uint32 pcicore_pcieserdesreg(void *pch, uint32 mdioslave, uint32 offset,
                                    uint32 mask, uint32 val);
#ifdef BCMDBG
extern int pcicore_dump_pcieregs(void *pch, struct bcmstrbuf *b);
#endif
#endif /* BCMINTERNAL */
#ifdef BCMDBG
extern uint32 pcie_lcreg(void *pch, uint32 mask, uint32 val);
extern void pcicore_dump(void *pch, struct bcmstrbuf *b);
#endif

extern bool pcicore_pmecap_fast(osl_t *osh);
extern void pcicore_pmeen(void *pch);
extern bool pcicore_pmeclr(void *pch);
#endif /* (BCMBUSTYPE == SI_BUS) || defined(BCMSDIO) || defined(BCMDHDUSB) */

#endif	/* _NICPCI_H */
