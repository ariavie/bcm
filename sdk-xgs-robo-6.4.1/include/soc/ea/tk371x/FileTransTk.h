/*
 * $Id: FileTransTk.h,v 1.3 Broadcom SDK $
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
 * File:     FileTransTk.h
 * Purpose: 
 *
 */
 
#ifndef _SOC_EA_FileTransTk_H
#define _SOC_EA_FileTransTk_H

#if defined(__cplusplus)
extern "C"  {
#endif

#include <soc/ea/tk371x/TkTypes.h>
#include <soc/ea/tk371x/Oam.h>


#define FILE_READ_REQ       0
#define FILE_WRITE_REQ      1

/* The maximum data we can transmit in an OAM frame (assuming max OAM frame
 * size of 1500). 
 * The overhead is: Oam header + 2 bytes of block number + 2 bytes of length
 */
#define MaxOftDataSize      (1470)

typedef enum {
    OamFileIdle,
    OamFileReading,
    OamFileWriting,
    OamFileNumStates
} OamFileState;

typedef struct {
    OamFileState    state;          /* reading from or writing to ONU */
    uint16          nextBlkNum;
    uint32          left;
    uint8         * currFilePos;    /* Pointer to current position in load */
    uint32          lastSend;
} OamFileSession;

typedef struct {
    OamFileState    state;
    uint16          blockNum;
    OamTkFileErr    lastErr;
    uint8         * buffer;
    uint32          size;
    uint32          maxSize;
    OamTkFileType   loadType;
} OamFileSessionInfo;

typedef enum TkFwUpgradMode {
    ExtSdkCtlMode   = 0,
    ExtAppCtlMode   = 1,
    UndefMode       = 0xff
} TkFwUpgradMode;

typedef struct TkOamFwUpgradeInstance_s {
    TkFwUpgradMode  fwUpGradeCtlMode;
    int             (*startFun) (uint8);
    int             (*finishedFun) (uint8, uint32, uint8);
    int             (*processFun) (uint8, uint16, uint8 *, uint16);
} TkOamFwUpgradeInstance_t;


/* send TK extension OAM file read or write request to the ONU and waiting for 
 * response.
 * ReqType : 0 - read, 1 - write
 */
int     TkExtOamFileTranReq (uint8 pathId, uint8 ReqType, uint8 loadType,
                OamTkFileAck * pTkFileAck);

int     TkExtOamFileSendData (uint8 pathId, uint16 blockNum, uint16 dataLen,
                uint8 * data, OamTkFileAck * pTkFileAck);

int     TkExtOamFileSendAck (uint8 pathId, uint8 err);

void    OamFileDone (uint8 pathId);

void    TkExtOamAppCtlModeFileSendAck (uint8 pathId, uint8 link, 
                uint16 blockNum, OamTkFileErr err);

void    TkExtOamAppCtlModeFileRdReq (uint8 pathId, uint8 link, uint8 * pData,
                uint16 len);

void    TkExtOamAppCtlModeFileWrReq (uint8 pathId, uint8 link, uint8 * pData,
                uint16 len);

void    TkExtOamAppCtlModeFileData (uint8 pathId, uint8 link, uint8 * pData,
                uint16 len);

void    TkExtOamAppCtlModeFileAck (uint8 pathId, uint8 link, uint8 * pData,
                uint16 len);

#if defined(__cplusplus)
}
#endif

#endif /* _SOC_EA_FileTransTk_H */
