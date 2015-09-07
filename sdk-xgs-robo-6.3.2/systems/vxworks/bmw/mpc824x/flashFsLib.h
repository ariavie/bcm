/* $Id: flashFsLib.h,v 1.5 2012/03/02 15:26:53 yaronm Exp $ */
#ifndef	FLASH_FS_LIB_H
#define	FLASH_FS_LIB_H

STATUS flashFsLibInit(void);
STATUS flashFsSync(void);
IMPORT STATUS sysHasDOC();

#define	FLASH_FS_NAME	((sysHasDOC()) ? "flsh:":"flash:")

#define FIOFLASHSYNC	0x10000
#define FIOFLASHINVAL	0x10001

#endif	/* !FLASH_FS_LIB_H */
