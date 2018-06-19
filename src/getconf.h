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

typedef struct {
    char *cf_label;
    char *cf_rhost;
    char *cf_rpath;
    int   cf_thresh;
    int   cf_nolimit;
} confent_t;

#ifndef _PATH_QUOTA_CONF
#define _PATH_QUOTA_CONF "/etc/quota.conf"
#endif

typedef ListIterator          conf_iterator_t;
typedef struct conf_struct *  conf_t;

#define CONF_MATCH_SUBDIR 1 /* conf_get_bylabel() flag: match mountpt subdir */
                            /* see util.c::match_path() */

conf_t            conf_init(char *path);
void              conf_fini(conf_t conf);
confent_t *       conf_get_bylabel(conf_t conf, char *label, int flags);
conf_iterator_t   conf_iterator_create(conf_t conf);
void              conf_iterator_destroy(conf_iterator_t itr);
confent_t *       conf_next(conf_iterator_t itr);

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
