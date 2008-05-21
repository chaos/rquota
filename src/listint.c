/*****************************************************************************\
 *  Copyright (C) 2001-2008 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Jim Garlick <garlick@llnl.gov>.
 *  UCRL-CODE-2003-005.
 *  
 *  This file is part of Quota, a remote quota program.
 *  For details, see <http://www.llnl.gov/linux/quota/>.
 *  
 *  Quota is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *  
 *  Quota is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *  
 *  You should have received a copy of the GNU General Public License along
 *  with Quota; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
\*****************************************************************************/

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "list.h"
#include "listint.h"

#ifdef WITH_LSD_NOMEM_ERROR_FUNC
#  undef lsd_nomem_error
   extern void * lsd_nomem_error(char *file, int line, char *mesg);
#else /* !WITH_LSD_NOMEM_ERROR_FUNC */
#  ifndef lsd_nomem_error
#    define lsd_nomem_error(file, line, mesg) (NULL)
#  endif /* !lsd_nomem_error */
#endif /* !WITH_LSD_NOMEM_ERROR_FUNC */

typedef enum { INVALID, SINGLE, RANGE } parsetype_t;

static unsigned long *dup_int(unsigned long u);
static parsetype_t   parse_int(char *s, unsigned long *u1p, unsigned long *u2p);
static int           listint_match(unsigned long *entry, unsigned long *key);

static parsetype_t
parse_int(char *s, unsigned long *u1p, unsigned long *u2p)
{
    unsigned long u1, u2;
    char *endptr;
    int rc = INVALID;

    u1 = strtoul(s, &endptr, 10);
    if (endptr == s)
        goto done;
    if (*endptr == '\0') {
        rc = SINGLE;
        goto done;
    }
    if (*endptr != '-')
        goto done;
    s = endptr + 1;
    u2 = strtoul(s, &endptr, 10);
    if (endptr == s)
        goto done;
    if (*endptr != '\0')
        goto done;
    rc = RANGE;
done:
    if ((rc == SINGLE || rc == RANGE) && u1p)
        *u1p = u1;
    if (rc == RANGE && u2p)
        *u2p = u2;
    return rc;
}

static unsigned long *
dup_int(unsigned long u)
{
    unsigned long *up = malloc(sizeof(unsigned long));

    if (up == NULL)
        return lsd_nomem_error(__FILE__, __LINE__, "dup_int");
    *up = u;
    return up;
}

static char *
dup_str(char *s)
{
    char *cpy = strdup(s);

    if (cpy == NULL)
        return lsd_nomem_error(__FILE__, __LINE__, "dup_str");
    return cpy;
}

/* Create a list of ints, parsing an string consisting of comma
 * separated numbers and ranges (mixed).  Return NULL on parse error.
 * N.B. we allow duplicates.
 */
List 
listint_create(char *s)
{
    List l;
    char *cpy, *t;
    unsigned long u, u1, u2;
    int rc;
  
    if (!(l = list_create((ListDelF)free)))
        return NULL; 
    if (!(cpy = dup_str(s))) {
        list_destroy(l);
        return NULL;
    }
    t = strtok(cpy, ",");
    while (t) {
        rc = parse_int(t, &u1, &u2);
        if (rc == INVALID) {
            list_destroy(l);
            l = NULL;
            break;
        } else if (rc == SINGLE) {
            list_append(l, dup_int(u1));
        } else { /* rc == RANGE */
            for (u = u1; u <= u2; u++)
                list_append(l, dup_int(u));
        }
        t = strtok(NULL, ",");
    }
    if (l && list_count(l) == 0) {
        list_destroy(l);
        l = NULL;
    }
    free(cpy);
    return l;
}

static int
listint_match(unsigned long *entry, unsigned long *key)
{
    return (*entry == *key);
}

int
listint_member(List l, unsigned long u)
{
    unsigned long *up;
 
    up = list_find_first(l, (ListFindF)listint_match, &u);
    return (up ? 1 : 0);
}

void
listint_destroy(List l)
{
    list_destroy(l);
}

#ifndef NDEBUG
void
listint_test(void)
{
    List l;

    l = listint_create("1,2,3");
    assert(l);
    assert(list_count(l) == 3);
    list_destroy(l);

    l = listint_create("1-1000");
    assert(l);
    assert(list_count(l) == 1000);
    list_destroy(l);

    l = listint_create("1-1000,1,2,50-100");
    assert(l);
    assert(list_count(l) == 1053);
    list_destroy(l);

    l = listint_create("0,1-1000,1005");
    assert(l);
    assert(list_count(l) == 1002);
    list_destroy(l);

    l = listint_create("");
    assert(l == NULL);

    l = listint_create(",1");
    assert(l);
    assert(list_count(l) == 1);
    list_destroy(l);

    l = listint_create("1,x");
    assert(l == NULL);
}
#endif

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
