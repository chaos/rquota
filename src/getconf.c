/*****************************************************************************\
 *  Copyright (C) 2001-2008 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Jim Garlick <garlick@llnl.gov>.
 *  UCRL-CODE-2003-005.
 *  
 *  This file is part of Quota, a remote NFS quota program.
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

/*
 * Code to parse the quota.conf file.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>

#include "list.h"
#include "getconf.h"
#include "util.h"

#define CONF_MAGIC 0x33221144
struct conf_struct {
    int conf_magic;
    List conf_ents; 
};

/*
 * Helper for getconfent().  Parse out a field within a string.
 * Replace separators with NULL's and return the field or NULL if at end.
 * 	p (IN/OUT)	string pointer - moved past next separator
 * 	sep (IN)	separator character
 *	RETURN		the base string (now \0 terminated at sep position)
 */
static char *
next_field(char **str, char sep)
{
    char *rv = *str;

    while (**str != '\0' && **str != sep && **str != '\n')
        (*str)++;

    *(*str)++ = '\0';

    return rv;
}

static void
zap_comment(char *str)
{
    char *p = strchr(str, '#');

    if (p)
        *p = '\0';
}

static void
zap_trailing_spaces(char *str)
{
    char *p = &str[strlen(str) - 1];

    while (p >= str && isspace(*p))
        *p-- = '\0';
}

/*
 * Read/parse the next configuration file entry.
 * 	RETURN		config file entry (caller must free)
 */
static confent_t *
getconfent(FILE *f)
{
    static char buf[BUFSIZ];
    char *thresh, *label, *rhost, *rpath, *p;
    confent_t *e = NULL;

    while (fgets(buf, BUFSIZ, f)) {
        zap_comment(buf);
        zap_trailing_spaces(buf);
        if (strlen(buf) > 0) {
            p = buf;
            label = next_field(&p, ':');
            rhost = next_field(&p, ':');
            rpath = next_field(&p, ':');
            thresh = next_field(&p, ':');

            e = (confent_t *)xmalloc(sizeof(confent_t));
            e->cf_label = xstrdup(label);
            e->cf_rhost = xstrdup(rhost);
            e->cf_rpath = xstrdup(rpath);
            e->cf_thresh = thresh ? strtoul(thresh, NULL, 10) : 0;
            break;
        }
    }

    return e;
}

static void
freeconfent(confent_t *e)
{
    if (e) {
        if (e->cf_label)
            free(e->cf_label);
        if (e->cf_rhost)
            free(e->cf_rhost);
        if (e->cf_rpath)
            free(e->cf_rpath);
        free(e);
    }
}

conf_t
conf_init(char *path)
{
    conf_t cp; 
    FILE *f = stdin;
    confent_t *e;

    if (strcmp(path, "-") != 0) {
        if (!(f = fopen(path, "r"))) {
            perror(path);
            exit(1);
        }
    } 

    cp = (conf_t)xmalloc(sizeof(struct conf_struct));
    cp->conf_magic = CONF_MAGIC;
    cp->conf_ents = list_create((ListDelF)freeconfent);

    while ((e = getconfent(f)))
        list_append(cp->conf_ents, e);

    if (strcmp(path, "-") != 0)
        fclose(f);
        
    return cp;
}

void 
conf_fini(conf_t cp)
{
    assert(cp);
    assert(cp->conf_magic == CONF_MAGIC);

    cp->conf_magic = 0;
    list_destroy(cp->conf_ents);
    free(cp);
}

conf_iterator_t
conf_iterator_create(conf_t cp)
{
    assert(cp);
    assert(cp->conf_magic == CONF_MAGIC);

    return list_iterator_create(cp->conf_ents);
}

void
conf_iterator_destroy(conf_iterator_t itr)
{
    list_iterator_destroy(itr);
}

confent_t *
conf_next(conf_iterator_t itr)
{
    return list_next(itr);
}

confent_t *
conf_get_bylabel(conf_t cp, char *label, int flags)
{
    conf_iterator_t itr;
    confent_t *e;

    assert(cp);
    assert(cp->conf_magic == CONF_MAGIC);

    itr = conf_iterator_create(cp);
    while ((e = conf_next(itr))) {
        if ((flags & CONF_MATCH_SUBDIR)) {
            if (match_path(label, e->cf_label))
                break;
        } else {
            if (!strcmp(label, e->cf_label))
                break;
        }
    }
    conf_iterator_destroy(itr);

    return e;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
