/*
 * $Id: kconfig.c,v 1.2 Broadcom SDK $
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
 * Configuration Variable Support
 *
 */

#include <linux/string.h>
#include <linux/ctype.h>

#include <sal/core/alloc.h>
#include <sal/core/spl.h>
#include <sal/types.h>

typedef	uint16	sc_hash_t;

/*
 * Speed up searches by first checking if a hash matches.
 * It encodes the first and last character, as well as string length.
 */
#define	SC_HASH(_n, _h)	do { \
	int _l = strlen(_n); \
	_h = (_n[0] << 8) | (_n[_l-1] << 4) | _l; \
	} while (0)

typedef struct sc_s {
    struct sc_s *sc_next;
    char	*sc_name;
    char	*sc_value;
    sc_hash_t	sc_hash;
} sc_t;

static sc_t *kconfig_list = NULL;

#define	FREE_SC(_s)                                     \
if (_s) {                                               \
    if ((_s)->sc_name) sal_free((_s)->sc_name);         \
    if ((_s)->sc_value) sal_free((_s)->sc_value);       \
    sal_free(_s);                                       \
}

/*
 * Function:
 *	kconfig_find
 * Purpose:
 *	Find a config entry for the specified name.
 * Parameters:
 *	name - name of variable to recover
 * Returns:
 *	NULL - not found
 *	!NULL - pointer to value
 */

static sc_t *
kconfig_find(const char *name)
{
    sc_t	*sc;
    sc_hash_t	hash;

    SC_HASH(name, hash);
    for (sc = kconfig_list; sc != NULL; sc = sc->sc_next) {
	if (sc->sc_hash != hash) {
	    continue;
	}
	if (strcmp(sc->sc_name, name) == 0) {
	    return sc;
	}
    }
    return NULL;
}

/*
 * Function:
 *	kconfig_get
 * Purpose:
 *	Recover a sal configuration variable
 * Parameters:
 *	name - name of variable to recover
 * Returns:
 *	NULL - not found
 *	!NULL - pointer to value
 */

char *
kconfig_get(const char *name)
{
    sc_t	*sc;

    sc = kconfig_find(name);
    return sc ? sc->sc_value : NULL;
}

/*
 * Function:
 *	kconfig_get_next
 * Purpose:
 *	Recover a sal configuration variables in order.
 * Parameters:
 *	name - (IN/OUT) name of last variable recovered.
 *	value- (OUT) value of variable recovered.
 * Returns:
 *	0 - variable recovered.
 *	-1 - All complete, not more variables.
 */

int
kconfig_get_next(char **name, char **value)
{
    sc_t	*sc;

    if (*name) {
	sc = kconfig_find(*name);
	sc = sc->sc_next;
    } else {
	sc = kconfig_list;
    }
    if (sc) {
	*name = sc->sc_name;
	*value = sc->sc_value;
	return 0;
    } else {
	return -1;
    }
}

/*
 * Function:
 *	kconfig_set
 * Purpose:
 *	Set a sal configuration variable
 * Parameters:
 *	name - name of variable to set
 *	value - name of value; can be NULL to delete a variable
 * Returns:
 *	0 - found and changed
 *	-1 - not found or out of memory
 */

int
kconfig_set(char *name, char *value)
{
    sc_t	*sc, *psc;
    char	*newval;
    sc_hash_t	hash;

    SC_HASH(name, hash);
    psc = NULL;
    for (sc = kconfig_list; sc != NULL; psc = sc, sc = sc->sc_next) {
	if (hash != sc->sc_hash) {
	    continue;
	}
	if (strcmp(sc->sc_name, name) == 0) {
	    break;
	}
    }

    if (sc != NULL) {		/* found */
	if (value == NULL) {	/* delete */
	    if (psc == NULL) {
		kconfig_list = sc->sc_next;
	    } else {
		psc->sc_next = sc->sc_next;
	    }
	    FREE_SC(sc);
	    return 0;
	} else {		/* replace */
	    newval = sal_alloc(strlen(value) + 1, "config value");
	    if (newval == NULL) {
		return -1;
	    }
	    strcpy(newval, value);
	    sal_free(sc->sc_value);
	    sc->sc_value = newval;
	    return 0;
	}
    }

    /* not found, add */
    if ((sc = sal_alloc(sizeof(sc_t), "config set")) == NULL) {
	return -1;
    }

    sc->sc_name = sal_alloc(strlen(name) + 1, "config name");
    sc->sc_value = sal_alloc(strlen(value) + 1, "config value");

    if (sc->sc_name == NULL || sc->sc_value == NULL) {
	FREE_SC(sc);
	return -1;
    }

    strcpy(sc->sc_name, name);
    strcpy(sc->sc_value, value);
    sc->sc_hash = hash;

    sc->sc_next = kconfig_list;
    kconfig_list = sc;

    return 0;
}
