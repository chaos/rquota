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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>
#include <lustre/liblustreapi.h>

#include "getconf.h"
#include "getquota.h"

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

int
getquota_lustre(char *fsname, uid_t uid, char *mnt, quota_t *q)
{
    static time_t now = 0;
    struct if_quotactl qctl;
    int rc;

    if (time(&now) < 0) {
        fprintf(stderr, "%s: time: %s\n", fsname, strerror(errno));
        return -1;
    }

    memset(&qctl, 0, sizeof(qctl));
    qctl.qc_cmd = LUSTRE_Q_GETQUOTA;
    qctl.qc_id = uid;
    rc = llapi_quotactl(mnt, &qctl);
    if (rc) {
        fprintf(stderr, "%s: llapi_quotactl: %s\n", fsname, strerror(errno));
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

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
