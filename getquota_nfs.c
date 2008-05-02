#include <ctype.h>
#include <pwd.h>
#include <rpc/rpc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/param.h>          /* MAXHOSTNAMELEN */
#include <assert.h>
#include <errno.h>

#include "rquota.h"
#include "getconf.h"
#include "getquota.h"
#include "util.h"

#define QUIRK_NETAPP  1 /* (uint32_t)(-1) for any limit == no quota */
#define QUIRK_DEC     0 /* 2 block block limits == no quota */

/* Normalize reply from quirky servers.
 */
static void
workaround_quirks(struct rquota *rq)
{
#if QUIRK_NETAPP
#define NETAPP_NOQUOTA (0xffffffff) /* (uint32_t)(-1) */
    if (rq->rq_bsoftlimit == NETAPP_NOQUOTA)
        rq->rq_bsoftlimit = 0;
    if (rq->rq_bhardlimit == NETAPP_NOQUOTA)
        rq->rq_bhardlimit = 0;
    if (rq->rq_fsoftlimit == NETAPP_NOQUOTA)
        rq->rq_fsoftlimit = 0;
    if (rq->rq_fhardlimit == NETAPP_NOQUOTA)
        rq->rq_fhardlimit = 0;
#endif
#if QUIRK_DEC
    if (rq->rq_bhardlimit == 2 && rq->rq_bsoftlimit == 2)
        rq->rq_bhardlimit = rq->rq_bsoftlimit = 0;
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

int
getquota_nfs(char *fsname, uid_t uid, char *rhost, char *rpath, quota_t *q)
{
    static char lhost[MAXHOSTNAMELEN+1] = "";
    getquota_args args;
    getquota_rslt *result;
    CLIENT *cl;
    int rc = -1; /* fail */

    if (lhost[0] == '\0') {
        if (gethostname(lhost, sizeof(lhost)) < 0) {
            fprintf(stderr, "%s: gethostbyname %s\n", fsname, strerror(errno));
            goto done;
        }
    }

    cl = clnt_create(rhost, RQUOTAPROG, RQUOTAVERS, "udp");
    if (cl == NULL) {
        clnt_pcreateerror(fsname);
        goto done;
    }

    /* Gnat48: authunix_create_default() fails if in >16 groups (Tru64),
     * so call authunix_create() with empty supplementary group list.
     */
    cl->cl_auth = authunix_create(lhost, uid, getgid(), 0, NULL);
    if (cl->cl_auth == NULL) {
        fprintf(stderr, "%s: authunix_create failed\n", fsname);
        goto done;
    }

    args.gqa_pathp  = rpath;
    args.gqa_uid    = uid;
    result = rquotaproc_getquota_1(&args, cl);

    if (result == NULL) {
        clnt_perror(cl, fsname);
        goto done;
    }
    if (result->gqr_status == Q_NOQUOTA) {
        fprintf(stderr, "%s: no quota\n", fsname);
        goto done;
    }
    if (result->gqr_status == Q_EPERM) {
        fprintf(stderr, "%s: permission denied\n", fsname);
        goto done;
    }
    if (result->gqr_status != Q_OK) {
        fprintf(stderr, "%s: unknown error: %d\n", fsname, result->gqr_status);
        goto done;
    }
    rc = 0;

    if (q) {
        struct rquota *rq = &result->getquota_rslt_u.gqr_rquota;

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

#ifdef STAND
int
main(int argc, char *argv[])
{
    quota_t q;
    int rc;
    char tmp[16];

    if (argc != 3) {
        fprintf(stderr, "Usage: nquota rhost rpath\n");
        exit(1);
    }
    snprintf(tmp, sizeof(tmp), "%s:%s", argv[1], argv[2]);
    rc = getquota_nfs(tmp, getuid(), argv[1], argv[2], &q);
    if (rc == 0) {
        printf("q_bytes_used     %llu (%s)\n", q.q_bytes_used,
                                    size2str(q.q_bytes_used, tmp, 16));
        printf("q_bytes_softlim  %llu (%s)\n", q.q_bytes_softlim,
                                    size2str(q.q_bytes_softlim, tmp, 16));
        printf("q_bytes_hardlim  %llu (%s)\n", q.q_bytes_hardlim,
                                    size2str(q.q_bytes_hardlim, tmp, 16));
        printf("q_bytes_state    %s\n", q.q_bytes_state == UNDER ? "under"
                          : q.q_bytes_state == NOTSTARTED ? "not started"
                          : q.q_bytes_state == STARTED ? "started" : "expired");
        if (q.q_bytes_state == STARTED) 
            printf("q_bytes_secleft  %llu\n", q.q_bytes_secleft);

        printf("q_files_used     %llu\n", q.q_files_used);
        printf("q_files_softlim  %llu\n", q.q_files_softlim);
        printf("q_files_hardlim  %llu\n", q.q_files_hardlim);
        printf("q_files_state    %s\n", q.q_files_state == UNDER ? "under"
                          : q.q_files_state == NOTSTARTED ? "not started"
                          : q.q_files_state == STARTED ? "started" : "expired");
        if (q.q_files_state == STARTED)
            printf("q_files_secleft  %llu\n", q.q_files_secleft);

    }
    exit(rc);
}
#endif

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
