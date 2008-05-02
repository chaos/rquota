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

#ifdef STAND
#include "util.h"
int
main(int argc, char *argv[])
{
    quota_t q;
    int rc = 0;
    char tmp[16];

    if (argc != 2) {
        fprintf(stderr, "Usage: lquota mntpt\n");
        exit(1);
    }
    rc = getquota_lustre(argv[1], getuid(), argv[1], &q);
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
