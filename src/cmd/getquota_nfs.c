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
#include <ctype.h>
#include <pwd.h>
#include <rpc/rpc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/param.h>
#include <netdb.h>
#include <assert.h>
#include <errno.h>

#include "src/libutil/util.h"
#include "src/librpc/rquota.h"

#include "getquota.h"
#include "getquota_private.h"

#define QUIRK_NETAPP  1 /* (uint32_t)(-1) for any limit == no quota */
#define QUIRK_DEC     0 /* 2 block block limits == no quota */

extern char *prog;
extern int debug;

double quota_nfs_timeout = 2.5;         // default sunrpc 25s
double quota_nfs_retry_timeout = 0.5;   // default sunrpc 5s

/* Normalize reply from quirky servers.
 */
static void
workaround_quirks(struct rquota *rq)
{
#if QUIRK_NETAPP
#define NETAPP_NOQUOTA (0xffffffff) /* (uint32_t)(-1) */
    if (rq->rq_bsoftlimit == NETAPP_NOQUOTA) {
        if (debug)
            printf("quirk: rq_bsoftlimit = NETAPP_NOQUOTA\n");
        rq->rq_bsoftlimit = 0;
    }
    if (rq->rq_bhardlimit == NETAPP_NOQUOTA) {
        if (debug)
            printf("quirk: rq_bhardlimit = NETAPP_NOQUOTA\n");
        rq->rq_bhardlimit = 0;
    }
    if (rq->rq_fsoftlimit == NETAPP_NOQUOTA) {
        if (debug)
            printf("quirk: rq_fsoftlimit = NETAPP_NOQUOTA\n");
        rq->rq_fsoftlimit = 0;
    }
    if (rq->rq_fhardlimit == NETAPP_NOQUOTA) {
        if (debug)
            printf("quirk: rq_fhardlimit = NETAPP_NOQUOTA\n");
        rq->rq_fhardlimit = 0;
    }
#endif
#if QUIRK_DEC
    if (rq->rq_bhardlimit == 2 && rq->rq_bsoftlimit == 2) {
        if (debug)
            printf("quirk: rq_bhardlimit = rq_bsoftlimit = 2 (DEC)\n");
        rq->rq_bhardlimit = rq->rq_bsoftlimit = 0;
    }
#endif
}

static qstate_t
set_state(unsigned long long used, unsigned long long soft,
          unsigned long long hard, unsigned long timeleft)
{
    qstate_t state;

    if (!hard && !soft)
        state = NONE;
    else if (hard && used > hard)
        state = EXPIRED;
    else if (soft && used > soft)
        state = (long)timeleft > 0 ? STARTED : NOTSTARTED;
    else
        state = UNDER;

    return state;
}

static void tv_double (double t, struct timeval *tv)
{
        tv->tv_sec = t;
            tv->tv_usec = (t - tv->tv_sec)*1E6;
}

int
quota_get_nfs(uid_t uid, quota_t q)
{
    static char lhost[MAXHOSTNAMELEN+1] = "";
    uid_t myuid = geteuid();
    getquota_args args;
    getquota_rslt *result;
    CLIENT *cl = NULL;
    struct timeval tv;
    int rc = -1; /* fail */

    assert(q->q_magic == QUOTA_MAGIC);
    if (myuid != 0 && myuid != uid) {
        fprintf(stderr, "%s: only root can query someone else's quota\n", prog);
        goto done;
    }

    /* just do this once and cache the result */
    if (lhost[0] == '\0') {
        if (gethostname(lhost, sizeof(lhost)) < 0) {
            fprintf(stderr, "%s: gethostbyname %s\n", prog, strerror(errno));
            goto done;
        }
    }

    cl = clnt_create(q->q_rhost, RQUOTAPROG, RQUOTAVERS, "udp");
    if (cl == NULL) {
        fprintf(stderr, "%s: %s\n", prog, clnt_spcreateerror(q->q_rhost));
        goto done;
    }

    /* Gnat48: authunix_create_default() fails if in >16 groups (Tru64),
     * so call authunix_create() with empty supplementary group list.
     */
    cl->cl_auth = authunix_create(lhost, uid, getgid(), 0, NULL);
    if (cl->cl_auth == NULL) {
        fprintf(stderr, "%s: %s\n", prog, clnt_sperror(cl, "authunix"));
        goto done;
    }

    /* github issue #7 - alter default RPC timeouts
     * of 5s retry timeout, 25s total timeout
     */
    tv_double (quota_nfs_retry_timeout, &tv);
    if (!clnt_control (cl, CLSET_RETRY_TIMEOUT, (char *)&tv)) {
        fprintf(stderr, "%s: clnt_control CLSET_RETRY_TIMEOUT\n", prog);
        goto done;
    }
    tv_double (quota_nfs_timeout, &tv);
    if (!clnt_control (cl, CLSET_TIMEOUT, (char *)&tv)) {
        fprintf(stderr, "%s: clnt_control CLSET_TIMEOUT\n", prog);
        goto done;
    }

    args.gqa_pathp  = q->q_rpath;
    args.gqa_uid    = uid;
    result = rquotaproc_getquota_1(&args, cl);

    if (result == NULL) {
        fprintf(stderr, "%s: %s\n", prog, clnt_sperror(cl, q->q_rhost));
        goto done;
    }
    if (result->gqr_status == Q_NOQUOTA) {
        fprintf(stderr, "%s: rquota %s:%s: no quota\n", prog,
                q->q_rhost, q->q_rpath);
        goto done;
    }
    if (result->gqr_status == Q_EPERM) {
        fprintf(stderr, "%s: rquota %s:%s: permission denied\n",
                prog, q->q_rhost, q->q_rpath);
        goto done;
    }
    if (result->gqr_status != Q_OK) {
        fprintf(stderr, "%s: rquota %s:%s: unknown error: %d\n",
                prog, q->q_rhost, q->q_rpath, result->gqr_status);
        goto done;
    }
    rc = 0;

    if (q) {
        struct rquota *rq = &result->getquota_rslt_u.gqr_rquota;

        if (debug) {
            printf("%s:%s: rq_bsize=%llu rq_curblocks=%llu rq_bsoftlimit=%llu "
                   "rq_bhardlimit=%llu rq_btimeleft=%llu rq_curfiles=%llu "
                   "rq_fsoftlimit=%llu rq_fhardlimit=%llu rq_ftimeleft=%llu\n",
                   q->q_rhost, q->q_rpath,
                   (unsigned long long)rq->rq_bsize,
                   (unsigned long long)rq->rq_curblocks,
                   (unsigned long long)rq->rq_bsoftlimit,
                   (unsigned long long)rq->rq_bhardlimit,
                   (unsigned long long)rq->rq_btimeleft,
                   (unsigned long long)rq->rq_curfiles,
                   (unsigned long long)rq->rq_fsoftlimit,
                   (unsigned long long)rq->rq_fhardlimit,
                   (unsigned long long)rq->rq_ftimeleft
            );
        }

        workaround_quirks(rq);

        q->q_uid = uid;

        q->q_bytes_used = (unsigned long long)rq->rq_curblocks*rq->rq_bsize;
        q->q_bytes_softlim = (unsigned long long)rq->rq_bsoftlimit*rq->rq_bsize;
        q->q_bytes_hardlim = (unsigned long long)rq->rq_bhardlimit*rq->rq_bsize;
        q->q_bytes_state = set_state(q->q_bytes_used, q->q_bytes_softlim,
                                     q->q_bytes_hardlim, rq->rq_btimeleft);
        if (q->q_bytes_state == STARTED)
            q->q_bytes_secleft  = rq->rq_btimeleft;

        q->q_files_used     = rq->rq_curfiles;
        q->q_files_softlim  = rq->rq_fsoftlimit;
        q->q_files_hardlim  = rq->rq_fhardlimit;
        q->q_files_state = set_state(q->q_files_used, q->q_files_softlim,
                                     q->q_files_hardlim, rq->rq_ftimeleft);
        if (q->q_files_state == STARTED)
            q->q_files_secleft  = rq->rq_ftimeleft;
    }

done:
    if (cl != NULL) {
        if (cl->cl_auth != NULL)
            auth_destroy(cl->cl_auth);
        clnt_destroy(cl);
    }
    return rc;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
