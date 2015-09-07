/* $Id: nvramstubs.c,v 1.4 2012/03/02 15:27:42 yaronm Exp $ */
#include <stdio.h>

/* stub for flash-based nvram_get() function */
char*
nvram_get(const char *name)
{
    if (!strcmp("boardtype", name)) {
        return("0x1");
    }
    sysSerialPrintString("nvram_get:");
    sysSerialPrintString(name);
	return ((char*)0);
}

/* stub for flash-based nvram_getall() function */
int
nvram_getall(char *buf, int count)
{
    sysSerialPrintString("nvram_getall");
	return 0;
}
