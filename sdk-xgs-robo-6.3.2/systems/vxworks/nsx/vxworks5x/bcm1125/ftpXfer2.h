/* $Id: ftpXfer2.h,v 1.4 2012/03/02 15:28:51 yaronm Exp $ */
#ifndef FTPXFER2_H
#define FTPXFER2_H

STATUS ftpXfer2(char *host, char *user, char *passwd, char *acct,
		char *cmd, char *dirname, char *filename,
		int *pCtrlSock, int *pDataSock);

#endif	/* FTPXFER2_H */
