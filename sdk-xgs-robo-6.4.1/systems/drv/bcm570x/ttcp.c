/*
 * $Id: ttcp.c,v 1.3 Broadcom SDK $
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
 */

/*
 * A cheap version of the "ttcp" command supporting:
 * ttcp -ts and -tsu - transmit tcp and udp
 * ttcp -rs and -rsu - receive tcp and udp
 *
 * This program transmits a pattern, not a file.
 * This program only receives in sink mode.
 *
 * getopt() changed to getopt_r() to make ttcp() reentrant on VxWorks.
 *
 * Example invocations from the VxWorks shell:
 *
 * sp ttcp, "-r"
 * sp ttcp, "-t -n100000 172.16.40.26"
 *
 */

#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sockLib.h>
#include <socket.h>
#include <in.h>
#include <ctype.h>
#include <errnoLib.h>
#include <inetLib.h>

#undef DEBUG_SEM
#ifdef DEBUG_SEM
#include <semLib.h>
SEM_ID ttcpDebugSem;
#endif

typedef struct
    {
    char *place;
    char *optarg;
    int opterr;
    int optind;
    int optopt;
    } GOPT;

static void
pattern(char *cp, int cnt)
{
	char c;
	int cnt1;

	cnt1 = cnt;
	c = 0;
	while (cnt1-- > 0) {
		while (!isprint(c & 0x7f))
			c++;
		*cp++ = (c++ & 0x7f);
	}
	
}

#define	EMSG	""

static int getopt_r (GOPT * g, int argc, char **argv, char *ostr);
extern void syserr(char *s);

void
ttcp(char *line)
{
	int ac;
	char *av[32];
	char *buf;
        char *host;
	int c;
	int fd;
	int newfd;
	int cnt;
	int nbytes;
	int die;
	int err;
        int udp;
        int trans;
        int num;
        int len;
        int sinkmode;
        short port;
        struct sockaddr_in sinhim, sinme;
        struct sockaddr_in frominet;
        int fromlen;
        GOPT optstruct;
        GOPT *g = &optstruct;

printf("ttcp!");
#ifdef DEBUG_SEM
        ttcpDebugSem = semBCreate (0, SEM_EMPTY);
        semTake (ttcpDebugSem, WAIT_FOREVER);
#endif
	/* init globals with default values */
	udp = 0;
	trans = 1;
	num = 1024;
	len = 1024;
	sinkmode = 1;
	port = 2000;
	host = NULL;
	g->place = EMSG;
	g->opterr = 1;
	g->optind = 0; 

	/* chop the command line into whitespace-delimited strings */
	for (ac = 0; ac < 32; ) {
		av[ac] = line;
		while ((*line != ' ') && (*line != '\t') && (*line != '\0'))
			line++;
		ac++;
		if (*line == '\0')
			break;
		*line++ = '\0';
		while ((*line == ' ') || (*line == '\t'))
			line++;
		if (*line == '\0')
			break;
	}

	while ((c = getopt_r (g, ac, av, "rrtsun:l:p:")) != -1) {
		switch (c) {
		case 'l':
			len = atoi(g->optarg);
			break;

		case 'n':
			num = atoi(g->optarg);
			break;

		case 'p':
			port = atoi(g->optarg);
			break;

		case 'r':
			trans = 0;
			break;

		case 's':	/* nop */
			break;

		case 't':
			trans = 1;
			break;

		case 'u':
			udp = 1;
			break;
		}
	}

	bzero((char*)&sinme, sizeof (sinme));
	bzero((char*)&sinhim, sizeof (sinhim));

	sinme.sin_family = AF_INET;

	if (trans) {
		host = av[g->optind];

		if ((*host < '0') || (*host > '9')) {
			printf("only dot notation internet addresses currently supported\n");
			return;
		}
		sinhim.sin_family = AF_INET;
		sinhim.sin_addr.s_addr = inet_addr(host);
		sinhim.sin_port = htons(port);
printf("inet_addr, port = 0x%x, %d\n", inet_addr(host), port);
		sinme.sin_port = INADDR_ANY;
	}
	else
		sinme.sin_port = htons(port);

	if ((fd = socket(AF_INET, udp? SOCK_DGRAM: SOCK_STREAM, 0)) == ERROR) {
		syserr("socket");
		return;
	}

	if (!trans) {
		int val;

		val = 1;
		if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*) &val, sizeof (val)) == ERROR) {
			syserr("setsockopt: REUSEADDR");
			goto done;
		}
	}

	if (trans) {
printf(" connect! \n");
		if (connect(fd, (struct sockaddr*) &sinhim, sizeof (sinhim)) == ERROR) {
			syserr("connect failed");
			goto done;
		}
		if (!udp)
			printf("connected\n");
	}
	else {	/* rcvr */


		if (bind(fd, (struct sockaddr*) &sinme, sizeof (sinme)) == ERROR) {
			syserr("bind");
			goto done;
		}

		if (!udp && (listen(fd, 2) == ERROR)) {
			syserr("listen");
			goto done;
		}

		fromlen = sizeof (frominet);

		if (!udp && (newfd = accept(fd, (struct sockaddr*) &frominet, &fromlen)) == ERROR) {
			syserr("accept");
			goto done;
		}

		if (!udp)
			printf("ttcp-r: accept from %s\n", inet_ntoa(frominet.sin_addr));

		if (!udp) {
			close(fd);
			fd = newfd;
		}
	}

        buf = malloc (len);
        if (buf == NULL)
            {
            printf ("malloc failed, len = %d\n", len);
            return;
            }

	pattern(buf, len);

	nbytes = 0;

	if (trans) {

		if (udp)
			write(fd, buf, 4);	/* rcvr start */
		
		die = 0;
		while (num > 0) {
			cnt = write(fd, buf, len);
			if (cnt == ERROR) {
				err = errnoGet();
#if 0  /* Jimmy test this */
				if ((die < 100) && ((err == EMSGSIZE) || (err == ENOBUFS))) {
					die++;
					taskDelay(10);
					continue;
				}
#endif
				syserr("write");
				goto done;
			}
			else if (cnt != len) {
				syserr("short write");
				goto done;
			}
			nbytes += len;
			num--;
			die = 0;
		}

		if (udp)
			write(fd, buf, 4);	/* rcvr stop */
	}
	else {
		if (udp) {
			die = 0;
			while ((cnt = read(fd, buf, len)) > 0) {
				if (cnt == ERROR) {
					syserr("read");
					goto done;
				}
				if (cnt <= 4) {
					if (die)
						goto done;
					die = 1;
				}
				nbytes += cnt;
			}
		}
		else {
			while ((cnt = read(fd, buf, len)) > 0)
				nbytes += cnt;
			if (cnt == ERROR) {
				syserr("read");
				goto done;
			}
		}
	}

done:
	if (trans)
		printf("wrote %d bytes\n", nbytes);
	else
		printf("read %d bytes\n", nbytes);

	close(fd);
	return;
}

#define	BADCH ('?')

void
syserr(char *s)
{
	printf("%s: error %d\n", s, errnoGet());
}

static void
error(GOPT *g, char *pch)
{
	if (!g->opterr)
		return;
	printf("ttcp: %s: %c\n", pch, g->optopt);
}

int
getopt_r (GOPT *g, int argc, char **argv, char *ostr)
{
        register char *oli;                /* option letter list index */

        if (!*g->place) {
                /* update scanning pointer */
                if (g->optind >= argc || *(g->place = argv[g->optind]) != '-' || !*++g->place) {
                        return EOF; 
                }
                if (*g->place == '-') {
                        /* found "--" */
                        ++g->optind;
                        return EOF;
                }
        }

        /* option letter okay? */
        if ((g->optopt = (int)*g->place++) == (int)':'
                || !(oli = strchr(ostr, g->optopt))) {
                if (!*g->place) {
                        ++g->optind;
                }
                error(g, "illegal option");
                return BADCH;
        }
        if (*++oli != ':') {        
                /* don't need argument */
                g->optarg = NULL;
                if (!*g->place)
                        ++g->optind;
        } else {
                /* need an argument */
                if (*g->place) {
                        g->optarg = g->place;           /* no white space */
                } else  if (argc <= ++g->optind) {
                        /* no arg */
                        g->place = EMSG;
                        error(g, "option requires an argument");
                        return BADCH;
                } else {
                        g->optarg = argv[g->optind];   /* white space */
                }
                g->place = EMSG;
                ++g->optind;
        }
        return g->optopt;                     /* return option letter */
}
