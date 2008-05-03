/*****************************************************************************\
 *  $Id: quota.c 7324 2008-05-02 14:44:44Z garlick $
 *****************************************************************************
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

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "getconf.h"
#include "getquota.h"

int
getquota(confent_t *c, char *fsname, uid_t uid, quota_t *q)
{
    int rc;

    if (!strcmp(c->cf_host, "lustre")) {
#if WITH_LUSTRE
        rc = getquota_lustre(fsname, uid, c->cf_path, q);
#else   
        fprintf(stderr, "%s: this quota program was not built with lustre support\n", fsname);
        rc = 1;
#endif
    } else {
#if WITH_NFS
        rc = getquota_nfs(fsname, uid, c->cf_host, c->cf_path, q);
#else
        fprintf(stderr, "%s: this quota program was not built with NFS support\n", fsname);
        rc = 1;
#endif
    }
    return rc;
}
