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
#if HAVE_LIBLUSTREAPI
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <time.h>
#include <errno.h>
#include <dlfcn.h>
#include <lustre/lustreapi.h>
#ifndef QUOTABLOCK_SIZE
#define QUOTABLOCK_SIZE (1 << 10)
#endif
#include <assert.h>

#include "list.h"
#include "util.h"
#include "getquota.h"
#include "getquota_private.h"

extern char *prog;

static qstate_t
set_state(unsigned long long used, unsigned long long soft,
          unsigned long long hard, unsigned long long xtim, time_t now)
{
    qstate_t state;

    if (!hard && !soft)
        state = NONE;
    else if (hard && used > hard)
        state = EXPIRED;
    else if (soft && used > soft)
        state = xtim > now ? STARTED : NOTSTARTED;
    else
        state = UNDER;

    return state;
}

int llapi_quotactl(char *mnt, struct if_quotactl *qctl)
{
    void *dso = NULL;
    int (*fun)(char *mnt, struct if_quotactl *qctl);
    int rc = -1;

    if (!(dso = dlopen("liblustreapi.so", RTLD_LAZY | RTLD_LOCAL))) {
        errno = EINVAL;
        goto done;
    }
    if (!(fun = dlsym (dso, "llapi_quotactl"))) {
        errno = EINVAL;
        goto done;
    }
    rc = fun(mnt, qctl);
done:
    if (dso)
        dlclose(dso);
    return rc;
}

int
quota_get_lustre(uid_t uid, quota_t q)
{
    time_t now = 0;
    struct if_quotactl qctl;
    struct statfs f;
    int rc;

    assert(q->q_magic == QUOTA_MAGIC);

    /* additional 'fs not mounted' error hanlding per chaos bz 1100/issue 1 */
    if (statfs (q->q_rpath, &f) < 0) {
        if (errno == ENOENT)
            fprintf (stderr, "%s: %s is not mounted\n", prog, q->q_rpath);
        else
            fprintf (stderr, "%s: %s %s\n", prog, q->q_rpath, strerror (errno));
        return -1;
    }
    if (f.f_type != LL_SUPER_MAGIC) {
        fprintf (stderr, "%s: %s is not mounted\n", prog, q->q_rpath);
        return -1;
    }
    if (time(&now) < 0) {
        fprintf(stderr, "%s: time: %s\n", prog, strerror(errno));
        return -1;
    }
    memset(&qctl, 0, sizeof(qctl));
    qctl.qc_cmd = LUSTRE_Q_GETQUOTA;
    qctl.qc_id = uid;
    rc = llapi_quotactl(q->q_rpath, &qctl);
    if (rc) {
        fprintf(stderr, "%s: llapi_quotactl %s: %s\n",
                        prog, q->q_rpath, strerror(errno));
        return rc;
    }
    if (q) {
        struct obd_dqblk *dqb = &qctl.qc_dqblk;

        q->q_uid            = uid;

        q->q_bytes_used     = dqb->dqb_curspace;
        q->q_bytes_softlim  = dqb->dqb_bsoftlimit * QUOTABLOCK_SIZE;
        q->q_bytes_hardlim  = dqb->dqb_bhardlimit * QUOTABLOCK_SIZE;
        q->q_bytes_state = set_state(q->q_bytes_used, q->q_bytes_softlim,
                                     q->q_bytes_hardlim, dqb->dqb_btime, now);
        if (q->q_bytes_state == STARTED)
            q->q_bytes_secleft = dqb->dqb_btime - now;

        q->q_files_used     = dqb->dqb_curinodes;
        q->q_files_softlim  = dqb->dqb_isoftlimit;
        q->q_files_hardlim  = dqb->dqb_ihardlimit;
        q->q_files_state = set_state(q->q_files_used, q->q_files_softlim,
                                     q->q_files_hardlim, dqb->dqb_itime, now);
        if (q->q_files_state == STARTED)
            q->q_files_secleft = dqb->dqb_itime - now;
    }
    return 0;
}
#endif /* HAVE_LIBLUSTREAPI */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
